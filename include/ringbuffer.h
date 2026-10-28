
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_NUM_CHANNELS 2
#define AUDIO_BITS_PER_SAMPLE 24
#define AUDIO_BYTES_PER_SAMPLE 4  // 32bit aligned (24bit data + padding)
#define AUDIO_FRAME_SAMPLES (AUDIO_SAMPLE_RATE / 1000)  // 48 samples per frame
#define AUDIO_FRAME_BYTES (AUDIO_FRAME_SAMPLES * AUDIO_NUM_CHANNELS * AUDIO_BYTES_PER_SAMPLE)

#define RINGBUF_FRAMES (8)
#define TOTAL_SAMPLES (RINGBUF_FRAMES * AUDIO_FRAME_SAMPLES)

typedef struct {
    int32_t buffer[RINGBUF_FRAMES * AUDIO_FRAME_BYTES];
    volatile size_t read;
    volatile size_t write;
} ringbuffer_t;

bool ringbuffer_push(ringbuffer_t *buffer, int32_t *data, size_t size);
bool ringbuffer_pop(ringbuffer_t *buffer, int32_t *data, size_t size);
size_t ringbuffer_capacity(ringbuffer_t *buffer);
size_t ringbuffer_size(ringbuffer_t *buffer);

#ifdef __cplusplus
}
#endif
