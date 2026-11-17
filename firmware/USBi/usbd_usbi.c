/* usbd_usbi.c - vendor-specific USB class (control-pipe only) for the USBi emulator.
 * The real USBi's bulk endpoints (EP2/4 OUT, EP6/8 IN) are declared in the descriptor but the
 * original firmware never services them (their interrupt vectors are empty), so they are not
 * opened here. (The F103 has no EP8 anyway.) */
#include "usbd_usbi.h"
#include "usbd_core.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"
#include "usbi_proto.h"
#include "usbi_hw.h"
#include "stm32f1xx_hal.h"

#ifndef USBI_BUF_SIZE
#define USBI_BUF_SIZE 4096   /* largest control data stage accepted (override with -DUSBI_BUF_SIZE=...) */
#endif
#ifndef DEVICE_FS
#define DEVICE_FS 0   /* device id passed to USBD_Init (CubeMX normally defines this in usbd_desc.h) */
#endif

USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;

static uint8_t xbuf[USBI_BUF_SIZE];
static usbi_setup_t pend;

/* Configuration descriptor copied from the real firmware (full-speed variant):
 * 1 interface, class 0xFF, 4 bulk endpoints, bus powered, 500 mA. */
__ALIGN_BEGIN static uint8_t cfg_desc[46] __ALIGN_END = {
  0x09, 0x02, 0x2E, 0x00, 0x01, 0x01, 0x00, 0x80, 0xFA,
  0x09, 0x04, 0x00, 0x00, 0x04, 0xFF, 0x00, 0x00, 0x00,
  0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
  0x07, 0x05, 0x04, 0x02, 0x40, 0x00, 0x00,
  0x07, 0x05, 0x86, 0x02, 0x40, 0x00, 0x00,
  0x07, 0x05, 0x88, 0x02, 0x40, 0x00, 0x00,
};

static uint8_t USBI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)   { return USBD_OK; }
static uint8_t USBI_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) { return USBD_OK; }

static uint8_t USBI_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  if ((req->bmRequest & USB_REQ_TYPE_MASK) != USB_REQ_TYPE_VENDOR) {
    USBD_CtlError(pdev, req); return USBD_FAIL;
  }
  usbi_setup_t s = { req->bmRequest, req->bRequest, req->wValue, req->wIndex, req->wLength };

  if (req->bmRequest & 0x80) {                              /* device -> host */
    if (req->wLength > USBI_BUF_SIZE) { USBD_CtlError(pdev, req); return USBD_FAIL; }
    int n = usbi_vendor_in(&s, xbuf, USBI_BUF_SIZE);
    if (n < 0) { USBD_CtlError(pdev, req); return USBD_FAIL; }
    if (n > req->wLength) n = req->wLength;
    USBD_CtlSendData(pdev, xbuf, (uint32_t)n);
  } else if (req->wLength) {                                /* host -> device, data stage */
    if (req->wLength > USBI_BUF_SIZE) { USBD_CtlError(pdev, req); return USBD_FAIL; }
    pend = s;
    USBD_CtlPrepareRx(pdev, xbuf, req->wLength);
  } else {                                                  /* no data stage */
    if (usbi_vendor_out(&s, NULL, 0) < 0) { USBD_CtlError(pdev, req); return USBD_FAIL; }
    USBD_CtlSendStatus(pdev);
  }
  return USBD_OK;
}

/* data stage complete (device must be in the CONFIGURED state: CyUSB selects config 1 on open) */
static uint8_t USBI_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  usbi_vendor_out(&pend, xbuf, pend.wLength);
  return USBD_OK;
}

static uint8_t *USBI_GetCfg(uint16_t *length) { *length = sizeof cfg_desc; return cfg_desc; }

USBD_ClassTypeDef USBD_USBI = {
  .Init = USBI_Init, .DeInit = USBI_DeInit, .Setup = USBI_Setup,
  .EP0_RxReady = USBI_EP0_RxReady,
  .GetHSConfigDescriptor = USBI_GetCfg, .GetFSConfigDescriptor = USBI_GetCfg,
  .GetOtherSpeedConfigDescriptor = USBI_GetCfg,
};

void USBI_Start(void)
{
  usbi_hw_init();
  /* Force re-enumeration (Blue Pill has a fixed D+ pull-up): pull PA12 low for 20 ms */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef g = {0};
  g.Pin = GPIO_PIN_12; g.Mode = GPIO_MODE_OUTPUT_PP; g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOA, &g);
  HAL_Delay(20);
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);

  USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
  USBD_RegisterClass(&hUsbDeviceFS, &USBD_USBI);
  USBD_Start(&hUsbDeviceFS);
}
