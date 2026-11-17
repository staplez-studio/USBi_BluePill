/* usbi_proto.c - translate USBi vendor requests into I2C / SPI bus operations.
 *
 * Parameter layout (verified against the firmware's setup handler):
 *   wValue low byte : device address. I2C: 7-bit address (wire byte = wValueL<<1 | R/W).
 *                     SPI: first byte on the wire = wValueL<<1 | (1 for B3, else 0).
 *   wIndex low byte : 0 = I2C, nonzero = SPI
 *   wIndex high byte: SPI chip-select 1..5 (ignored for I2C)
 *   B2 data         : bytes sent after the address byte, then STOP / CS release
 *   B3 data         : same, but bus is held (no STOP / CS stays low) for a following B4
 *   B4 wLength      : number of bytes to read
 * SigmaDSP register read = B3 (2-byte sub-address) then B4 (N bytes), then B6 for status. */
#include "usbi_proto.h"
#include "usbi_hw.h"
#include <string.h>

static uint8_t st_wr, st_rd;

#if USBI_LOG
static void lputs(const char *s) { while (*s) usbi_log_putc(*s++); }
static void lhex(uint32_t v, int digits)
{ static const char h[] = "0123456789ABCDEF"; for (int i = (digits - 1) * 4; i >= 0; i -= 4) usbi_log_putc(h[(v >> i) & 15]); }
/* one line per request: "<dir> rt=40 rq=B2 v=0034 i=0000 l=0004 : 00 10 AA BB [!]"   ! = not handled */
static void logreq(char dir, const usbi_setup_t *s, const uint8_t *d, uint16_t n, int unhandled)
{
  usbi_log_putc(dir); lputs(" rt="); lhex(s->bmRequestType, 2); lputs(" rq="); lhex(s->bRequest, 2);
  lputs(" v="); lhex(s->wValue, 4); lputs(" i="); lhex(s->wIndex, 4); lputs(" l="); lhex(s->wLength, 4);
  if (d && n) { lputs(" :"); for (uint16_t i = 0; i < n && i < 16; i++) { usbi_log_putc(' '); lhex(d[i], 2); } if (n > 16) lputs(" .."); }
  if (unhandled) lputs(" !");
  lputs("\r\n");
}
#else
#define logreq(...) ((void)0)
#endif
static enum { HELD_NONE, HELD_I2C, HELD_SPI } held;

static uint8_t i2c_addr(const usbi_setup_t *s, int rd)
{ return (uint8_t)((s->wValue << 1) | (rd ? 1u : 0u)); }

static uint8_t i2c_write(const usbi_setup_t *s, const uint8_t *d, uint16_t n, int hold)
{
  int r = usbi_i2c_start();
  if (r) { held = HELD_NONE; return USBI_ST_BERR; }
  r = usbi_i2c_wr(i2c_addr(s, 0));
  for (uint16_t i = 0; !r && i < n; i++) r = usbi_i2c_wr(d[i]);
  if (r) { usbi_i2c_stop(); held = HELD_NONE; return (uint8_t)r; }
  if (hold) held = HELD_I2C; else { usbi_i2c_stop(); held = HELD_NONE; }
  return USBI_ST_OK;
}

static uint8_t i2c_read(const usbi_setup_t *s, uint8_t *buf, uint16_t n)
{
  memset(buf, 0, n);
  int r = usbi_i2c_start();               /* repeated START if bus was held by B3 */
  if (r) { held = HELD_NONE; return USBI_ST_BERR; }
  r = usbi_i2c_wr(i2c_addr(s, 1));
  for (uint16_t i = 0; !r && i < n; i++) r = usbi_i2c_rd(&buf[i], i != (uint16_t)(n - 1));
  usbi_i2c_stop();
  held = HELD_NONE;
  return r ? (uint8_t)r : USBI_ST_OK;
}

static uint8_t spi_write(const usbi_setup_t *s, const uint8_t *d, uint16_t n, int hold)
{
  usbi_spi_cs_release();
  usbi_spi_cs_assert((uint8_t)(s->wIndex >> 8));
  usbi_spi_wr((uint8_t)((s->wValue << 1) | (hold ? 1u : 0u)));
  for (uint16_t i = 0; i < n; i++) usbi_spi_wr(d[i]);
  if (hold) held = HELD_SPI; else { usbi_spi_cs_release(); held = HELD_NONE; }
  return USBI_ST_OK;
}

static uint8_t spi_read(uint8_t *buf, uint16_t n)
{
  memset(buf, 0, n);
  if (held != HELD_SPI) return USBI_ST_BERR;  /* original firmware: undefined; needs B3 first */
  for (uint16_t i = 0; i < n; i++) buf[i] = usbi_spi_rd();
  usbi_spi_cs_release();
  held = HELD_NONE;
  return USBI_ST_OK;
}

int usbi_vendor_out(const usbi_setup_t *s, const uint8_t *d, uint16_t len)
{
  int spi = (s->wIndex & 0xFF) != 0;
  logreq('O', s, d, len, s->bRequest < USBI_REQ_NOP || s->bRequest > USBI_REQ_AUX2);
  switch (s->bRequest) {
  case USBI_REQ_NOP:   return 0;
  case USBI_REQ_WRITE:
    st_wr = spi ? spi_write(s, d, len, 0) : i2c_write(s, d, len, 0);
    usbi_led_mode(spi); return 0;
  case USBI_REQ_ADDR:
    st_wr = spi ? spi_write(s, d, len, 1) : i2c_write(s, d, len, 1);
    usbi_led_mode(spi); return 0;
  case USBI_REQ_RESET: usbi_target_reset(); return 0;
  case USBI_REQ_AUX1:  usbi_aux1((s->wValue & 0xFF) != 0); return 0;
  case USBI_REQ_AUX2:  usbi_aux2((s->wValue & 0xFF) != 1); return 0;
  default:             return -1;
  }
}

int usbi_vendor_in(const usbi_setup_t *s, uint8_t *buf, uint16_t max)
{
  int spi = (s->wIndex & 0xFF) != 0;
  uint16_t n = s->wLength > max ? max : s->wLength;
  logreq('I', s, NULL, 0, s->bRequest != USBI_REQ_B1 && (s->bRequest < USBI_REQ_READ || s->bRequest > USBI_REQ_RD_STATUS));
  switch (s->bRequest) {
  case USBI_REQ_B1:        buf[0] = 0; return 1;
  case USBI_REQ_READ:
    st_rd = spi ? spi_read(buf, n) : i2c_read(s, buf, n);
    usbi_led_mode(spi); return n;
  case USBI_REQ_WR_STATUS: buf[0] = st_wr; st_wr = 0; return 1;
  case USBI_REQ_RD_STATUS: buf[0] = st_rd; st_rd = 0; return 1;
  default:                 return -1;
  }
}
