/* board.h - board-specific pin choices.
 *   USBI_BOARD_PJ26 = 1 : DX-SMART DX-PJ26 (STM32F103C8T6 + CH340 USB-serial). User LED on PB11.
 *   USBI_BOARD_PJ26 = 0 : standard Blue Pill. User LED on PC13.
 * The LED pin and SPI chip-select 5 swap places so that every allowed pin stays in use:
 *   allowed pins: PA0-PA7, PB0, PB1, PB10, PB11, PC13, PC14, PC15  (+ USB PA11/PA12, OLED PB13/PB14). */
#pragma once
#include "stm32f1xx_hal.h"

#ifndef USBI_BOARD_PJ26
#define USBI_BOARD_PJ26 1
#endif

#if USBI_BOARD_PJ26
  #define HB_PORT   GPIOB          /* heartbeat = user LED */
  #define HB_PIN    GPIO_PIN_11
  #define CS5_PORT  GPIOC          /* SPI chip-select 5 (slow pin, light load) */
  #define CS5_PIN   GPIO_PIN_13
#else
  #define HB_PORT   GPIOC
  #define HB_PIN    GPIO_PIN_13
  #define CS5_PORT  GPIOB
  #define CS5_PIN   GPIO_PIN_11
#endif
