/* oled.h - text-only driver for a 128x64 I2C OLED (SH1106 or SSD1306), 8 lines x 21 characters. */
#pragma once
#include <stdint.h>
#define OLED_COLS 21
#define OLED_LINES 8
int  oled_init(void);                              /* returns 1 if a display answered, else 0 */
void oled_text_line(uint8_t line, const char *s);  /* redraws only if the text changed */
