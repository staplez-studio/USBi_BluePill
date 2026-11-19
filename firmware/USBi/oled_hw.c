#include "oled_hw.h"
#include "stm32f1xx_hal.h"
#ifndef OLED_I2C_HZ
#define OLED_I2C_HZ 400000u
#endif
#define OSCL GPIO_PIN_13
#define OSDA GPIO_PIN_14
#define SCL_H() (GPIOB->BSRR = OSCL)
#define SCL_L() (GPIOB->BRR  = OSCL)
#define SDA_H() (GPIOB->BSRR = OSDA)
#define SDA_L() (GPIOB->BRR  = OSDA)
#define SDA_RD() ((GPIOB->IDR & OSDA) != 0)
static uint32_t half;
static inline void dly(uint32_t c) { uint32_t t = DWT->CYCCNT; while ((uint32_t)(DWT->CYCCNT - t) < c) { } }

void oled_hw_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  GPIOB->BSRR = OSCL | OSDA;
  GPIO_InitTypeDef g = {0};
  g.Pin = OSCL | OSDA; g.Mode = GPIO_MODE_OUTPUT_OD; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &g);                         /* open-drain: the display module has its own pull-ups */
  half = SystemCoreClock / (2u * OLED_I2C_HZ);
}
static int wbyte(uint8_t b)
{
  for (int i = 0; i < 8; i++) { if (b & 0x80) SDA_H(); else SDA_L(); b <<= 1; dly(half); SCL_H(); dly(half); SCL_L(); }
  SDA_H(); dly(half); SCL_H(); dly(half);
  int nack = SDA_RD();
  SCL_L();
  return nack;
}
int oled_hw_write(uint8_t a8, const uint8_t *d, uint16_t n)
{
  int err;
  SDA_H(); SCL_H(); dly(half); SDA_L(); dly(half); SCL_L();     /* START */
  err = wbyte(a8);
  for (uint16_t i = 0; i < n; i++) err |= wbyte(d[i]);
  SDA_L(); dly(half); SCL_H(); dly(half); SDA_H(); dly(half);   /* STOP */
  return err;
}
