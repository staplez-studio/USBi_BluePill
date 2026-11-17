/* usbi_proto.h - USBi (0456:7031) vendor protocol, recovered from the USBi
 * firmware image's own request dispatcher (see USBI_PROTOCOL.md).
 * All requests are vendor type, device recipient: bmRequestType 0x40 (OUT) / 0xC0 (IN). */
#pragma once
#include <stdint.h>

#define USBI_REQ_NOP        0xB0  /* ack, nothing                                  */
#define USBI_REQ_B1         0xB1  /* IN, 1 byte, always 0x00 (purpose unknown)      */
#define USBI_REQ_WRITE      0xB2  /* OUT: bus write, then release bus (STOP / CS hi) */
#define USBI_REQ_ADDR       0xB3  /* OUT: bus write, bus HELD (sub-address phase)    */
#define USBI_REQ_READ       0xB4  /* IN : bus read (repeated START / CS held)        */
#define USBI_REQ_WR_STATUS  0xB5  /* IN : 1 status byte for last B2/B3, then cleared */
#define USBI_REQ_RD_STATUS  0xB6  /* IN : 1 status byte for last B4, then cleared    */
#define USBI_REQ_RESET      0xB7  /* pulse reset low; host lib calls it CMD_RESET     */
#define USBI_REQ_AUX1       0xB8  /* wValueL != 0 -> output high; likely CMD_POWERBOARD */
#define USBI_REQ_AUX2       0xB9  /* wValueL == 1 -> output low; likely CMD_LED (active low) */

/* Status byte values returned by B5/B6 (from the firmware's I2C state machine) */
#define USBI_ST_OK    1
#define USBI_ST_MORE  2   /* original: more EP0 data pending; never produced here */
#define USBI_ST_NACK  3   /* slave did not ACK                                      */
#define USBI_ST_BERR  4   /* bus error                                              */

typedef struct {
  uint8_t  bmRequestType, bRequest;
  uint16_t wValue, wIndex, wLength;
} usbi_setup_t;

/* IN: fill buf (max bytes available), return length to send, or <0 to STALL. */
int usbi_vendor_in (const usbi_setup_t *s, uint8_t *buf, uint16_t max);
/* OUT: data/len = completed data stage (len may be 0). Return <0 to STALL. */
int usbi_vendor_out(const usbi_setup_t *s, const uint8_t *data, uint16_t len);
