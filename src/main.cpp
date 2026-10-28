
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

static ringbuffer_t rx_buffer = {0};

static int32_t scratch_in[64 * 2];

static int32_t test_samples[48 * 2];
static uint32_t sample_phase = 0;
static uint32_t last_audio_ms = 0;

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
void audio_task(void)
{
    uint32_t sampling_rate;
    uint8_t bit_rate;
    uint8_t channels;

    usb_audio_get_config(&sampling_rate, &bit_rate, &channels);

    if (sampling_rate != 48000 || channels != 2)
        return;

    uint32_t now = board_millis();

    if (now == last_audio_ms)
        return;

    last_audio_ms = now;

    for (size_t i = 0; i < 48; i++) {
        uint32_t phase = sample_phase;

        int32_t triangle;

        if (phase < 48) {
            triangle = (int32_t)(
                -0x40000000LL +
                (int64_t)phase * (0x80000000LL / 48));
        } else {
            triangle = (int32_t)(
                0x40000000LL -
                (int64_t)(phase - 48) * (0x80000000LL / 48));
        }

        int32_t square =
            (phase < 48) ? 0x40000000 : -0x40000000;

        test_samples[2 * i]     = triangle;
        test_samples[2 * i + 1] = square;

        sample_phase = (sample_phase + 1) % 96;
    }

    tud_audio_write(test_samples, sizeof(test_samples));
}

int main(void)
{
    set_sys_clock_khz(240000, true);

    board_init();

    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };

    tusb_init(BOARD_TUD_RHPORT, &dev_init);
    board_init_after_tusb();

    while (1) {
        tud_task();
        audio_task();
        led_task();

        static uint32_t last_print = 0;
        uint32_t now = board_millis();

        if (now - last_print > 1000) {
            last_print = now;

            if (tud_cdc_connected()) {
                tud_cdc_write_str("RP2350 raw USB audio loopback\r\n");
                tud_cdc_write_flush();
            }
        }
    }
}
