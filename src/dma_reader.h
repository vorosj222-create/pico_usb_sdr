
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hardware/dma.h"
#include "hardware/pio.h"

class DMAReader
{
private:
    static DMAReader *readers[NUM_DMA_CHANNELS];
    static void __isr dma_handler();

    volatile uint32_t *buffers[2];
    const size_t buffer_words;

    volatile uint8_t current_buffer = 0;
    volatile uint8_t ready_mask = 0;

    void handle_irq();

    DMAReader(const DMAReader &) = delete;

public:
    const PIO pio;
    const uint8_t sm;
    const uint8_t dma_channel;

    DMAReader(
        PIO pio,
        uint8_t sm,
        volatile uint32_t *buffer_a,
        volatile uint32_t *buffer_b,
        size_t buffer_words);

    volatile uint32_t *getCompletedBuffer();

    ~DMAReader();
};
