
/*
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "led.h"
#include "pico/stdlib.h"
#include "ringbuffer.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "hardware/pio.h"
#include "dma_reader.h"
#include "i2s_master_clock.h"
#include "i2s_master_input.h"

static ringbuffer_t rx_buffer = {0};

static int32_t scratch_in[64 * 2];

static int32_t test_samples[48 * 2];
static uint32_t sample_phase = 0;
static uint32_t last_audio_ms = 0;

static constexpr size_t I2S_BUFFER_WORDS = 48 * 2;

alignas(32) static uint32_t i2s_buffer_a[I2S_BUFFER_WORDS];
alignas(32) static uint32_t i2s_buffer_b[I2S_BUFFER_WORDS];
static int32_t usb_samples[I2S_BUFFER_WORDS];

static uint32_t usb_write_failures = 0;

void led_task(void)
{
    led_update();
}

/* USB audio output: TinyUSB handles packet transmission. */
bool tud_audio_tx_done_pre_load_cb(
    uint8_t rhport,
    uint8_t itf,
    uint8_t ep_in,
    uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)itf;
    (void)ep_in;
    (void)cur_alt_setting;

    return true;
}

/* USB audio input: receive samples into the RX buffer. */
bool tud_audio_rx_done_pre_read_cb(
    uint8_t rhport,
    uint16_t n_bytes_received,
    uint8_t func_id,
    uint8_t ep_out,
    uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;

    if (ringbuffer_capacity(&rx_buffer) == 0)
        return true;

    uint16_t rx_size = tud_audio_read(scratch_in, n_bytes_received);

    if (rx_size != n_bytes_received)
        return true;

    ringbuffer_push(
        &rx_buffer,
        scratch_in,
        rx_size / sizeof(int32_t));

    return true;
}

/* Generate 500 Hz triangle (left) and square (right), 48 kHz stereo. */
void audio_task(DMAReader &reader)
{
    uint32_t sampling_rate;
    uint8_t bit_rate;
    uint8_t channels;

    usb_audio_get_config(&sampling_rate, &bit_rate, &channels);

    if (sampling_rate != 48000 || channels != 2)
        return;

    volatile uint32_t *samples = reader.getCompletedBuffer();

    if (samples == nullptr)
        return;

    for (size_t i = 0; i < I2S_BUFFER_WORDS; i++)
        usb_samples[i] = (int32_t)((uint32_t)samples[i] << 1);

    uint16_t written = tud_audio_write(usb_samples, sizeof(usb_samples));

    if (written != sizeof(usb_samples))
    usb_write_failures++;

    (void)written;
}

int main(void)
{
    set_sys_clock_khz(192000, true);

    board_init();

    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };

    tusb_init(BOARD_TUD_RHPORT, &dev_init);
    board_init_after_tusb();

    PIO pio = pio0;

I2SMasterClock clock(
    pio,
    pio_claim_unused_sm(pio, true),
    16,
    48000,
    256
);

I2SMasterInput input(
    pio,
    pio_claim_unused_sm(pio, true),
    20,
    17,
    48000,
    32
);

DMAReader reader(
    pio,
    input.sm,
    i2s_buffer_a,
    i2s_buffer_b,
    I2S_BUFFER_WORDS
);

clock.enable();
input.enable();

    while (1) {
        tud_task();
        audio_task(reader);
        led_task();

        static uint32_t last_print = 0;
        uint32_t now = board_millis();

        if (now - last_print > 1000) {
            last_print = now;

            if (tud_cdc_connected()) {
    tud_cdc_write_str("USB audio write failures: ");
    char msg[16];
snprintf(msg, sizeof(msg), "%lu\r\n",
         (unsigned long)usb_write_failures);
tud_cdc_write_str(msg);
    tud_cdc_write_flush();
}
        }
    }
}
