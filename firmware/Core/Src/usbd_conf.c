/* usbd_conf.c - glue between ST's USB device library and the F103 USB full-speed peripheral.
 * Derived from ST's STM32CubeF1 USB device examples (BSD-3-Clause), trimmed for a control-only device. */
#include "stm32f1xx_hal.h"
#include "usbd_core.h"
#include "usbd_def.h"

PCD_HandleTypeDef hpcd;

void HAL_PCD_MspInit(PCD_HandleTypeDef *h)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  g.Pin = GPIO_PIN_11 | GPIO_PIN_12;            /* USB D- / D+ */
  g.Mode = GPIO_MODE_AF_INPUT; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &g);
  __HAL_RCC_USB_CLK_ENABLE();
  HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *h) { __HAL_RCC_USB_CLK_DISABLE(); }

/* ---- PCD -> USB device library callbacks ---- */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *h)  { USBD_LL_SetupStage((USBD_HandleTypeDef *)h->pData, (uint8_t *)h->Setup); }
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *h, uint8_t ep) { USBD_LL_DataOutStage((USBD_HandleTypeDef *)h->pData, ep, h->OUT_ep[ep].xfer_buff); }
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *h, uint8_t ep)  { USBD_LL_DataInStage((USBD_HandleTypeDef *)h->pData, ep, h->IN_ep[ep].xfer_buff); }
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *h)        { USBD_LL_SOF((USBD_HandleTypeDef *)h->pData); }
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *h)
{
  USBD_LL_SetSpeed((USBD_HandleTypeDef *)h->pData, USBD_SPEED_FULL);
  USBD_LL_Reset((USBD_HandleTypeDef *)h->pData);
}
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *h)    { USBD_LL_Suspend((USBD_HandleTypeDef *)h->pData); }
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *h)     { USBD_LL_Resume((USBD_HandleTypeDef *)h->pData); }
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *h, uint8_t ep) { USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)h->pData, ep); }
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *h, uint8_t ep)  { USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)h->pData, ep); }
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *h)    { USBD_LL_DevConnected((USBD_HandleTypeDef *)h->pData); }
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *h) { USBD_LL_DevDisconnected((USBD_HandleTypeDef *)h->pData); }

/* ---- USB device library -> PCD ---- */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
  hpcd.Instance = USB;
  hpcd.Init.dev_endpoints = 8;
  hpcd.Init.speed = PCD_SPEED_FULL;
  hpcd.Init.low_power_enable = DISABLE;
  hpcd.Init.lpm_enable = DISABLE;
  hpcd.Init.battery_charging_enable = DISABLE;
  hpcd.pData = pdev;
  pdev->pData = &hpcd;
  if (HAL_PCD_Init(&hpcd) != HAL_OK) return USBD_FAIL;
  HAL_PCDEx_PMAConfig(&hpcd, 0x00, PCD_SNG_BUF, 0x40);   /* EP0 OUT */
  HAL_PCDEx_PMAConfig(&hpcd, 0x80, PCD_SNG_BUF, 0x80);   /* EP0 IN  */
  return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)  { HAL_PCD_DeInit(pdev->pData); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)   { HAL_PCD_Start(pdev->pData);  return USBD_OK; }
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)    { HAL_PCD_Stop(pdev->pData);   return USBD_OK; }
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep, uint8_t type, uint16_t mps)
{ HAL_PCD_EP_Open(pdev->pData, ep, mps, type); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep)  { HAL_PCD_EP_Close(pdev->pData, ep); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep)  { HAL_PCD_EP_Flush(pdev->pData, ep); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep)  { HAL_PCD_EP_SetStall(pdev->pData, ep); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep) { HAL_PCD_EP_ClrStall(pdev->pData, ep); return USBD_OK; }
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep)
{
  PCD_HandleTypeDef *h = (PCD_HandleTypeDef *)pdev->pData;
  return (ep & 0x80) ? h->IN_ep[ep & 0x7F].is_stall : h->OUT_ep[ep & 0x7F].is_stall;
}
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t addr) { HAL_PCD_SetAddress(pdev->pData, addr); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep, uint8_t *buf, uint32_t size)
{ HAL_PCD_EP_Transmit(pdev->pData, ep, buf, size); return USBD_OK; }
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep, uint8_t *buf, uint32_t size)
{ HAL_PCD_EP_Receive(pdev->pData, ep, buf, size); return USBD_OK; }
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep) { return HAL_PCD_EP_GetRxCount(pdev->pData, ep); }
void USBD_LL_Delay(uint32_t ms) { HAL_Delay(ms); }

static uint32_t static_pool[8];
void *USBD_static_malloc(uint32_t size) { (void)size; return static_pool; }
void  USBD_static_free(void *p) { (void)p; }
