/* ui.c - status screen glue (main loop only; never called from the USB interrupt). */
#include "ui.h"
#include "ui_format.h"
#include "oled.h"
#include "usbi_stats.h"
#include "usbd_usbi.h"
#include "stm32f1xx_hal.h"

static int have_oled;
void ui_init(void) { have_oled = oled_init(); }

void ui_poll(void)
{
  static uint32_t last;
  if (!have_oled || (uint32_t)(HAL_GetTick() - last) < 100) return;
  last = HAL_GetTick();
  usbi_stats_t s; char lines[8][22];
  usbi_stats_get(&s);
  ui_format(lines, &s, hUsbDeviceFS.dev_state, HAL_GetTick());
  for (uint8_t i = 0; i < 8; i++) oled_text_line(i, lines[i]);
}
