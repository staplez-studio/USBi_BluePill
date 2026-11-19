#pragma once
#include "usbd_def.h"
extern USBD_ClassTypeDef USBD_USBI;
extern USBD_HandleTypeDef hUsbDeviceFS;
void USBI_Start(void);   /* call from main() USER CODE 2 instead of MX_USB_DEVICE_Init() */
