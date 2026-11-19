/* ui_format.h - turns the statistics into the 8 text lines shown on the OLED (pure C, host-testable). */
#pragma once
#include "usbi_stats.h"
#include <stdint.h>
/* usb_state: 0 = not started, 1 = default, 2 = addressed, 3 = configured, 4 = suspended */
void ui_format(char lines[8][22], const usbi_stats_t *s, uint8_t usb_state, uint32_t uptime_ms);
