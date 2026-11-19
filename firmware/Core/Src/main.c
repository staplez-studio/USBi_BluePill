/* main.c - USBi (0456:7031) emulator for the STM32F103C8 "Blue Pill" */
#include "stm32f1xx_hal.h"
#include "usbd_usbi.h"
#include "ui.h"
#include "board.h"

static void Error_Handler(void) { __disable_irq(); while (1) { } }

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};
  RCC_PeriphCLKInitTypeDef pclk = {0};

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;        /* 8 MHz crystal on the Blue Pill */
  osc.HSEState = RCC_HSE_ON;
  osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL = RCC_PLL_MUL9;                      /* 72 MHz */
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV2;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

  pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;      /* 72 MHz / 1.5 = 48 MHz for USB */
  pclk.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();   /* heartbeat LED: PB11 (DX-PJ26) or PC13 (Blue Pill) */
  GPIO_InitTypeDef g = {0};
  g.Pin = HB_PIN; g.Mode = GPIO_MODE_OUTPUT_PP; g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HB_PORT, &g);

  USBI_Start();                                       /* bus pins, re-enumeration, USB stack */
  ui_init();                                          /* OLED status screen (skipped if no display answers) */

  uint32_t t = HAL_GetTick();
  for (;;) {
    if (HAL_GetTick() - t >= 500) { t += 500; HAL_GPIO_TogglePin(HB_PORT, HB_PIN); }
    ui_poll();
  }
}
