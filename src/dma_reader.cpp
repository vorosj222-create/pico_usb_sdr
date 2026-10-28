
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "dma_reader.h"

DMAReader *DMAReader::readers[NUM_DMA_CHANNELS] = {nullptr};

void __isr DMAReader::dma_handler()
{
    for (uint8_t channel = 0; channel < NUM_DMA_CHANNELS; channel++)
    {
        if (!dma_channel_get_irq0_status(channel))
            continue;

        dma_channel_acknowledge_irq0(channel);

        DMAReader *reader = readers[channel];

        if (reader != nullptr)
            reader->handle_irq();
    }
}

void DMAReader::handle_irq()
{
    uint8_t completed = current_buffer;
    uint8_t next = completed ^ 1u;

    ready_mask |= (1u << completed);

    // A következő puffer tartalmát a DMA felülírja.
    // Ezért a korábban ott maradt kész jelzést töröljük.
    ready_mask &= ~(1u << next);

    current_buffer = next;

    dma_channel_set_write_addr(
        dma_channel,
        buffers[next],
        false);

    dma_channel_set_trans_count(
        dma_channel,
        buffer_words,
        true);
}

DMAReader::DMAReader(
    PIO pio,
    uint8_t sm,
    volatile uint32_t *buffer_a,
    volatile uint32_t *buffer_b,
    size_t buffer_words)
    : pio(pio),
      sm(sm),
      dma_channel(dma_claim_unused_channel(true)),
      buffers{buffer_a, buffer_b},
      buffer_words(buffer_words)
{
    assert(buffer_a != nullptr);
    assert(buffer_b != nullptr);
    assert(buffer_words > 0);

    dma_channel_config config =
        dma_channel_get_default_config(dma_channel);

    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    channel_config_set_dreq(
        &config,
        pio_get_dreq(pio, sm, false));

    dma_channel_configure(
        dma_channel,
        &config,
        buffers[0],
        &pio->rxf[sm],
        buffer_words,
        false);

    readers[dma_channel] = this;

    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_channel);
}

volatile uint32_t *DMAReader::getCompletedBuffer()
{
    uint32_t irq_state = save_and_disable_interrupts();

    volatile uint32_t *result = nullptr;

    if (ready_mask & 1u)
    {
        ready_mask &= ~1u;
        result = buffers[0];
    }
    else if (ready_mask & 2u)
    {
        ready_mask &= ~2u;
        result = buffers[1];
    }

    restore_interrupts(irq_state);

    return result;
}

DMAReader::~DMAReader()
{
    dma_channel_set_irq0_enabled(dma_channel, false);
    dma_channel_abort(dma_channel);

    readers[dma_channel] = nullptr;

    dma_channel_unclaim(dma_channel);
}
