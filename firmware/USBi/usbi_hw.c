/* usbi_hw.c - bit-banged I2C + SPI for the USBi emulator (Blue Pill, STM32F103C8).
 * Bit-banging (like the original FX2 firmware) keeps everything free of HAL timeouts, so it is
 * safe to run inside the USB interrupt, and gives exact control of START/repeated START/CS.
 * Do NOT enable I2C1/SPI1 in CubeMX; this file configures the pins itself.
 *
 *   I2C : SCL PB6, SDA PB7  (open-drain; needs external pull-ups, 4.7k to 3.3V)
 *   SPI : SCLK PA5, MISO PA6, MOSI PA7;  CS1..CS5 = PB0, PB1, PB10, PB11, PB12
 *   RESET (B7) PB13 open-drain, AUX1 (B8) PB14, AUX2 (B9) PB15, LEDs PA1 (I2C) / PA2 (SPI)  */
#include "usbi_hw.h"
#include "stm32f1xx_hal.h"

#ifndef USBI_I2C_HZ
#define USBI_I2C_HZ 100000u
#endif
#ifndef USBI_SPI_HZ
#define USBI_SPI_HZ 500000u
#endif

#define SCL_PIN GPIO_PIN_6
#define SDA_PIN GPIO_PIN_7
#define SCL_H() (GPIOB->BSRR = SCL_PIN)
#define SCL_L() (GPIOB->BRR  = SCL_PIN)
#define SDA_H() (GPIOB->BSRR = SDA_PIN)
#define SDA_L() (GPIOB->BRR  = SDA_PIN)
#define SDA_RD() ((GPIOB->IDR & SDA_PIN) != 0)
#define SCL_RD() ((GPIOB->IDR & SCL_PIN) != 0)

#define SCLK_PIN GPIO_PIN_5
#define MISO_PIN GPIO_PIN_6
#define MOSI_PIN GPIO_PIN_7
#define SCLK_H() (GPIOA->BSRR = SCLK_PIN)
#define SCLK_L() (GPIOA->BRR  = SCLK_PIN)
#define MOSI_H() (GPIOA->BSRR = MOSI_PIN)
#define MOSI_L() (GPIOA->BRR  = MOSI_PIN)

static const uint16_t cs_pin[5] = { GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12 };
#define CS_ALL (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12)

static uint32_t cyc_us, i2c_half, spi_half;

static inline void dly(uint32_t c)
{ uint32_t t = DWT->CYCCNT; while ((uint32_t)(DWT->CYCCNT - t) < c) { } }

static void pins(GPIO_TypeDef *p, uint32_t pin, uint32_t mode, uint32_t pull)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = pin; g.Mode = mode; g.Pull = pull; g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(p, &g);
}

void usbi_hw_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cyc_us   = SystemCoreClock / 1000000u;
  i2c_half = SystemCoreClock / (2u * USBI_I2C_HZ);
  spi_half = SystemCoreClock / (2u * USBI_SPI_HZ);

  GPIOB->BSRR = SCL_PIN | SDA_PIN | GPIO_PIN_13;                 /* released / high first */
  pins(GPIOB, SCL_PIN | SDA_PIN | GPIO_PIN_13, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL);
  GPIOB->BSRR = CS_ALL | GPIO_PIN_14 | GPIO_PIN_15;
  pins(GPIOB, CS_ALL | GPIO_PIN_14 | GPIO_PIN_15, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
  GPIOA->BSRR = SCLK_PIN;                                        /* SPI mode 3: clock idles high */
  pins(GPIOA, SCLK_PIN | MOSI_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
  pins(GPIOA, MISO_PIN, GPIO_MODE_INPUT, GPIO_PULLUP);
  GPIOA->BSRR = GPIO_PIN_1 | GPIO_PIN_2;
  pins(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
#if USBI_LOG
  __HAL_RCC_AFIO_CLK_ENABLE(); __HAL_RCC_USART1_CLK_ENABLE();
  pins(GPIOA, GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_NOPULL);          /* USART1 TX = PA9 */
  USART1->BRR = HAL_RCC_GetPCLK2Freq() / 115200u;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
#endif
}

void usbi_log_putc(char c)
{
#if USBI_LOG
  uint32_t t = DWT->CYCCNT;
  while (!(USART1->SR & USART_SR_TXE)) if ((uint32_t)(DWT->CYCCNT - t) > cyc_us * 20000u) return;
  USART1->DR = (uint8_t)c;
#else
  (void)c;
#endif
}

/* ---------------- I2C ---------------- */
static int scl_release(void)
{
  SCL_H();
  uint32_t t = DWT->CYCCNT, lim = cyc_us * 2000u;               /* 2 ms clock-stretch limit */
  while (!SCL_RD()) if ((uint32_t)(DWT->CYCCNT - t) > lim) return -1;
  return 0;
}

int usbi_i2c_start(void)
{
  SDA_H(); dly(i2c_half);
  if (scl_release()) return 4;
  dly(i2c_half);
  if (!SDA_RD()) return 4;                                       /* someone holds SDA low */
  SDA_L(); dly(i2c_half); SCL_L(); dly(i2c_half);
  return 0;
}

void usbi_i2c_stop(void)
{ SDA_L(); dly(i2c_half); scl_release(); dly(i2c_half); SDA_H(); dly(i2c_half); }

int usbi_i2c_wr(uint8_t b)
{
  for (int i = 0; i < 8; i++) {
    if (b & 0x80) SDA_H(); else SDA_L();
    b <<= 1; dly(i2c_half);
    if (scl_release()) return 4;
    dly(i2c_half); SCL_L();
  }
  SDA_H(); dly(i2c_half);
  if (scl_release()) return 4;
  dly(i2c_half / 2);
  int nack = SDA_RD();
  dly(i2c_half / 2); SCL_L();
  return nack ? 3 : 0;
}

int usbi_i2c_rd(uint8_t *out, int ack)
{
  uint8_t v = 0;
  SDA_H();
  for (int i = 0; i < 8; i++) {
    dly(i2c_half);
    if (scl_release()) return 4;
    dly(i2c_half / 2);
    v = (uint8_t)((v << 1) | (SDA_RD() ? 1 : 0));
    dly(i2c_half / 2); SCL_L();
  }
  if (ack) SDA_L(); else SDA_H();
  dly(i2c_half);
  if (scl_release()) return 4;
  dly(i2c_half); SCL_L(); SDA_H();
  *out = v;
  return 0;
}

/* ---------------- SPI (mode 3, MSB first) ---------------- */
void usbi_spi_cs_assert(uint8_t idx)
{
  if (idx >= 1 && idx <= 5) { GPIOB->BRR = cs_pin[idx - 1]; dly(spi_half); }
}
void usbi_spi_cs_release(void) { GPIOB->BSRR = CS_ALL; }

void usbi_spi_wr(uint8_t b)
{
  for (int i = 0; i < 8; i++) {
    SCLK_L(); if (b & 0x80) MOSI_H(); else MOSI_L();
    b <<= 1; dly(spi_half); SCLK_H(); dly(spi_half);
  }
}

uint8_t usbi_spi_rd(void)
{
  uint8_t v = 0;
  MOSI_L();
  for (int i = 0; i < 8; i++) {
    SCLK_L(); dly(spi_half);
    v = (uint8_t)((v << 1) | ((GPIOA->IDR & MISO_PIN) ? 1 : 0));
    SCLK_H(); dly(spi_half);
  }
  return v;
}

/* ---------------- misc lines ---------------- */
void usbi_target_reset(void) { GPIOB->BRR = GPIO_PIN_13; dly(cyc_us * 1000u); GPIOB->BSRR = GPIO_PIN_13; }
void usbi_aux1(int level) { if (level) GPIOB->BSRR = GPIO_PIN_14; else GPIOB->BRR = GPIO_PIN_14; }
void usbi_aux2(int level) { if (level) GPIOB->BSRR = GPIO_PIN_15; else GPIOB->BRR = GPIO_PIN_15; }
void usbi_led_mode(int spi)
{ if (spi) { GPIOA->BRR = GPIO_PIN_2; GPIOA->BSRR = GPIO_PIN_1; } else { GPIOA->BRR = GPIO_PIN_1; GPIOA->BSRR = GPIO_PIN_2; } }
