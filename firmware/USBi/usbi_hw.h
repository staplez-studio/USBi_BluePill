/* usbi_hw.h - bit-banged I2C / SPI + control GPIOs (Blue Pill, STM32F103C8) */
#pragma once
#include <stdint.h>
#ifndef USBI_LOG
#define USBI_LOG 1   /* 1 = log every vendor request on USART1 TX (PA9, 115200 8N1). Set 0 for normal use. */
#endif
void usbi_log_putc(char c);
void usbi_hw_init(void);
/* I2C: return 0 = ok, 3 = NACK, 4 = bus error/timeout */
int  usbi_i2c_start(void);          /* START, or repeated START if bus is held */
void usbi_i2c_stop(void);
int  usbi_i2c_wr(uint8_t b);
int  usbi_i2c_rd(uint8_t *b, int ack);
/* SPI mode 3 (CPOL=1, CPHA=1), MSB first. idx 1..5 = CS1..CS5; other values assert nothing */
void usbi_spi_cs_assert(uint8_t idx);
void usbi_spi_cs_release(void);
void usbi_spi_wr(uint8_t b);
uint8_t usbi_spi_rd(void);
/* misc control lines */
void usbi_target_reset(void);
void usbi_aux1(int level);
void usbi_aux2(int level);
void usbi_led_mode(int spi);
