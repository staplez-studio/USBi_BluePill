/* oled.c - SH1106 / SSD1306 text driver.
 * Build options (add as -D... in the Makefile DEFS):
 *   OLED_SH1106 1|0   1 = SH1106 (typical 1.3" module, default), 0 = SSD1306 (typical 0.96")
 *   OLED_FLIP   1|0   1 = default orientation, 0 = rotate 180 degrees
 *   OLED_ADDR8  0x78  8-bit I2C address (0x78 = 7-bit 0x3C, 0x7A = 7-bit 0x3D)               */
#include "oled.h"
#include "oled_hw.h"
#include "font5x8.h"
#include <string.h>

#ifndef OLED_ADDR8
#define OLED_ADDR8 0x78
#endif
#ifndef OLED_SH1106
#define OLED_SH1106 1
#endif
#ifndef OLED_FLIP
#define OLED_FLIP 1
#endif
#define COL_OFF (OLED_SH1106 ? 2 : 0)    /* the SH1106 RAM is 132 columns wide, the glass shows 128 starting at column 2 */

static char shown[OLED_LINES][OLED_COLS + 1];
static int present;

static int send(uint8_t ctrl, const uint8_t *d, uint16_t n)
{
  uint8_t buf[132]; buf[0] = ctrl; memcpy(buf + 1, d, n);
  return oled_hw_write(OLED_ADDR8, buf, (uint16_t)(n + 1));
}
static void cmds(const uint8_t *c, uint16_t n) { send(0x00, c, n); }
static void set_pos(uint8_t page)
{
  uint8_t c[3] = { (uint8_t)(0xB0 | (page & 7)), (uint8_t)(COL_OFF & 0x0F), (uint8_t)(0x10 | (COL_OFF >> 4)) };
  cmds(c, 3);
}

int oled_init(void)
{
  static const uint8_t a[] = { 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40 };
#if OLED_SH1106
  static const uint8_t b[] = { 0xAD, 0x8B };                    /* internal DC-DC on */
#else
  static const uint8_t b[] = { 0x8D, 0x14, 0x20, 0x02 };        /* charge pump on, page addressing */
#endif
#if OLED_FLIP
  static const uint8_t c[] = { 0xA1, 0xC8 };
#else
  static const uint8_t c[] = { 0xA0, 0xC0 };
#endif
  static const uint8_t d[] = { 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6 };
  uint8_t off = 0xAE, on = 0xAF, zeros[128];
  oled_hw_init();
  present = (send(0x00, &off, 1) == 0);
  if (!present) return 0;
  cmds(a, sizeof a); cmds(b, sizeof b); cmds(c, sizeof c); cmds(d, sizeof d);
  memset(zeros, 0, sizeof zeros);
  for (uint8_t p = 0; p < OLED_LINES; p++) { set_pos(p); send(0x40, zeros, 128); }
  cmds(&on, 1);
  memset(shown, 0, sizeof shown);
  return 1;
}

void oled_text_line(uint8_t line, const char *s)
{
  char t[OLED_COLS + 1];
  uint8_t col[128];
  if (!present || line >= OLED_LINES) return;
  memset(t, ' ', OLED_COLS); t[OLED_COLS] = 0;
  for (int i = 0; i < OLED_COLS && s[i]; i++) t[i] = s[i];
  if (memcmp(t, shown[line], OLED_COLS + 1) == 0) return;       /* unchanged: skip the I2C traffic */
  memset(col, 0, sizeof col);
  for (int i = 0; i < OLED_COLS; i++) {
    unsigned ch = (unsigned char)t[i]; if (ch < 32 || ch > 126) ch = '?';
    memcpy(&col[i * 6], font5x8[ch - 32], 5);
  }
  set_pos(line); send(0x40, col, 128);
  memcpy(shown[line], t, OLED_COLS + 1);
}
