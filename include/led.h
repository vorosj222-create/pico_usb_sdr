
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BLINK_STREAMING = 25,     // 25 ms   : streaming data
    BLINK_NOT_MOUNTED = 250,  // 250 ms  : device not mounted
    BLINK_MOUNTED = 1000,     // 1000 ms : device mounted
    BLINK_SUSPENDED = 2500,   // 2500 ms : device is suspended
};

void led_set_blink_interval(uint32_t interval_ms);
void led_update(void);

#ifdef __cplusplus
}
#endif
