/* oled_hw.h - bit-banged I2C master for the status display on PB13 (SCL) / PB14 (SDA). */
#pragma once
#include <stdint.h>
void oled_hw_init(void);
/* START, addr8, bytes..., STOP. Returns 0 if every byte was acknowledged, nonzero otherwise. */
int  oled_hw_write(uint8_t addr8, const uint8_t *d, uint16_t n);
