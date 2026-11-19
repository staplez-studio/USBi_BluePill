/* usbi_hw.c - bit-banged I2C + SPI for the USBi emulator (Blue Pill, STM32F103C8).
 * Bit-banging (like the original FX2 firmware) keeps everything free of HAL timeouts, so it is
 * safe to run inside the USB interrupt, and gives exact control of START/repeated START/CS.
 *
 * PIN MAP: only these pins are used by the firmware:
 *   PA0-PA7, PB0, PB1, PB10, PB11, PC13, PC14, PC15
 *   (plus PA11/PA12, which the USB peripheral itself requires)
 *
 *   I2C   : SCL PA0, SDA PA1                (open-drain; needs external pull-ups to 3.3 V)
 *   SPI   : SCLK PA5, MISO PA6, MOSI PA7    (bit-banged, mode 3)
 *           CS1 PA4, CS2 PB0, CS3 PB1, CS4 PB10, CS5 PC13 (PB11 on Blue Pill)
 *   RESET (request B7): PA3, open-drain     (target needs a pull-up to 3.3 V)
 *   AUX1  (request B8): PC14                (slow, light-load output)
 *   AUX2  (request B9): PC15                (slow, light-load output)
 *   Heartbeat LED     : PB11 on DX-PJ26 / PC13 on Blue Pill (see board.h; swaps places with CS5)
 *   Debug log TX      : PA2 = USART2_TX (USBI_LOG_UART=2) or PA9 = USART1_TX (USBI_LOG_UART=1, DX-PJ26 on-board USB-serial)
 *   OLED (optional)   : PB13 = SCL, PB14 = SDA (oled_hw.c)
 * PC13-PC15 are limited by the chip: max 2 MHz and about 3 mA, and PC14/PC15 share the pins of the
 * Blue Pill's on-board 32.768 kHz crystal. They are used here only for slow, static outputs.
 * The two mode LEDs of the original USBi are not implemented (no spare pins). */
#include "usbi_hw.h"
#include "stm32f1xx_hal.h"
#include "board.h"

#ifndef USBI_I2C_HZ
#define USBI_I2C_HZ 100000u
#endif
#ifndef USBI_SPI_HZ
#define USBI_SPI_HZ 500000u
#endif

/* I2C on port A */
#define SCL_PIN GPIO_PIN_0
#define SDA_PIN GPIO_PIN_1
#define SCL_H() (GPIOA->BSRR = SCL_PIN)
#define SCL_L() (GPIOA->BRR  = SCL_PIN)
#define SDA_H() (GPIOA->BSRR = SDA_PIN)
#define SDA_L() (GPIOA->BRR  = SDA_PIN)
#define SDA_RD() ((GPIOA->IDR & SDA_PIN) != 0)
#define SCL_RD() ((GPIOA->IDR & SCL_PIN) != 0)

/* SPI on port A */
#define SCLK_PIN GPIO_PIN_5
#define MISO_PIN GPIO_PIN_6
#define MOSI_PIN GPIO_PIN_7
#define SCLK_H() (GPIOA->BSRR = SCLK_PIN)
#define SCLK_L() (GPIOA->BRR  = SCLK_PIN)
#define MOSI_H() (GPIOA->BSRR = MOSI_PIN)
#define MOSI_L() (GPIOA->BRR  = MOSI_PIN)

typedef struct { GPIO_TypeDef *port; uint16_t pin; } pin_t;
static const pin_t cs[5] = {
  { GPIOA, GPIO_PIN_4 }, { GPIOB, GPIO_PIN_0 }, { GPIOB, GPIO_PIN_1 }, { GPIOB, GPIO_PIN_10 }, { CS5_PORT, CS5_PIN }
};

#define RST_PIN  GPIO_PIN_3      /* port A, open-drain */
#define AUX1_PIN GPIO_PIN_14     /* port C */
#define AUX2_PIN GPIO_PIN_15     /* port C */

static uint32_t cyc_us, i2c_half, spi_half;

static inline void dly(uint32_t c)
{ uint32_t t = DWT->CYCCNT; while ((uint32_t)(DWT->CYCCNT - t) < c) { } }

static void pins(GPIO_TypeDef *p, uint32_t pin, uint32_t mode, uint32_t pull, uint32_t speed)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = pin; g.Mode = mode; g.Pull = pull; g.Speed = speed;
  HAL_GPIO_Init(p, &g);
}

void usbi_hw_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cyc_us   = SystemCoreClock / 1000000u;
  i2c_half = SystemCoreClock / (2u * USBI_I2C_HZ);
  spi_half = SystemCoreClock / (2u * USBI_SPI_HZ);

  /* levels first (released / idle), then switch the pins to output */
  GPIOA->BSRR = SCL_PIN | SDA_PIN | RST_PIN;
  pins(GPIOA, SCL_PIN | SDA_PIN | RST_PIN, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
  GPIOA->BSRR = SCLK_PIN;                                        /* SPI mode 3: clock idles high */
  pins(GPIOA, SCLK_PIN | MOSI_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
  pins(GPIOA, MISO_PIN, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
  for (int i = 0; i < 5; i++) {                                  /* chip-selects idle high */
    cs[i].port->BSRR = cs[i].pin;
    pins(cs[i].port, cs[i].pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL,
         cs[i].port == GPIOC ? GPIO_SPEED_FREQ_LOW : GPIO_SPEED_FREQ_HIGH);
  }
  GPIOC->BSRR = AUX1_PIN | AUX2_PIN;                             /* PC14/PC15: slow outputs only */
  pins(GPIOC, AUX1_PIN | AUX2_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
#if USBI_LOG
#if USBI_LOG_UART == 1
  __HAL_RCC_USART1_CLK_ENABLE();
  pins(GPIOA, GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);   /* USART1 TX = PA9 */
  USART1->BRR = HAL_RCC_GetPCLK2Freq() / 115200u;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
#else
  __HAL_RCC_USART2_CLK_ENABLE();
  pins(GPIOA, GPIO_PIN_2, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);   /* USART2 TX = PA2 */
  USART2->BRR = HAL_RCC_GetPCLK1Freq() / 115200u;
  USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
#endif
#endif
}

void usbi_log_putc(char c)
{
#if USBI_LOG
  USART_TypeDef *u = (USBI_LOG_UART == 1) ? USART1 : USART2;
  uint32_t t = DWT->CYCCNT;
  while (!(u->SR & USART_SR_TXE)) if ((uint32_t)(DWT->CYCCNT - t) > cyc_us * 20000u) return;
  u->DR = (uint8_t)c;
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
  if (idx >= 1 && idx <= 5) { cs[idx - 1].port->BRR = cs[idx - 1].pin; dly(spi_half); }
}
void usbi_spi_cs_release(void) { for (int i = 0; i < 5; i++) cs[i].port->BSRR = cs[i].pin; }

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
void usbi_target_reset(void) { GPIOA->BRR = RST_PIN; dly(cyc_us * 1000u); GPIOA->BSRR = RST_PIN; }
void usbi_aux1(int level) { if (level) GPIOC->BSRR = AUX1_PIN; else GPIOC->BRR = AUX1_PIN; }
void usbi_aux2(int level) { if (level) GPIOC->BSRR = AUX2_PIN; else GPIOC->BRR = AUX2_PIN; }
void usbi_led_mode(int spi) { (void)spi; }   /* mode LEDs not implemented: no spare pins in the allowed pin set */
