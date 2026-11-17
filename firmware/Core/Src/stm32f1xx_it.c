#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "usbd_conf.h"
void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { while (1) { } }
void SysTick_Handler(void)   { HAL_IncTick(); }
void USB_LP_CAN1_RX0_IRQHandler(void) { HAL_PCD_IRQHandler(&hpcd); }
