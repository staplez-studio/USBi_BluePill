/* usbi_desc.c - device + string descriptors identical to the real USBi (programmed, 0456:7031).
 * Replaces CubeMX's usbd_desc.c: exclude that file from the build (it defines the same FS_Desc). */
#include "usbd_core.h"
#include "usbd_ctlreq.h"

/* Copied byte-for-byte from the firmware: bcdUSB 2.00, class 0, EP0 64, VID 0x0456, PID 0x7031,
 * bcdDevice 0x0000, iManufacturer 1, iProduct 2, iSerialNumber 0, 1 configuration. */
__ALIGN_BEGIN static uint8_t dev_desc[18] __ALIGN_END = {
  0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
  0x56, 0x04, 0x31, 0x70, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01,
};
__ALIGN_BEGIN static uint8_t lang_desc[4] __ALIGN_END = { 0x04, 0x03, 0x09, 0x04 };
__ALIGN_BEGIN static uint8_t str_buf[64] __ALIGN_END;

static uint8_t *Dev (USBD_SpeedTypeDef sp, uint16_t *len) { *len = sizeof dev_desc;  return dev_desc;  }
static uint8_t *Lang(USBD_SpeedTypeDef sp, uint16_t *len) { *len = sizeof lang_desc; return lang_desc; }
static uint8_t *Manu(USBD_SpeedTypeDef sp, uint16_t *len) { USBD_GetString((uint8_t *)"Cypress", str_buf, len); return str_buf; }
static uint8_t *Prod(USBD_SpeedTypeDef sp, uint16_t *len) { USBD_GetString((uint8_t *)"EZ-USB",  str_buf, len); return str_buf; }
static uint8_t *Ser (USBD_SpeedTypeDef sp, uint16_t *len) { USBD_GetString((uint8_t *)"0",       str_buf, len); return str_buf; } /* never requested: index 0 */

USBD_DescriptorsTypeDef FS_Desc = {
  .GetDeviceDescriptor = Dev, .GetLangIDStrDescriptor = Lang,
  .GetManufacturerStrDescriptor = Manu, .GetProductStrDescriptor = Prod,
  .GetSerialStrDescriptor = Ser, .GetConfigurationStrDescriptor = Ser, .GetInterfaceStrDescriptor = Ser,
};
