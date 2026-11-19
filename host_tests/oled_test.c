/* Host test: protocol -> stats -> ui_format -> oled driver -> simulated SH1106 -> decoded text */
#include "usbi_proto.h"
#include "usbi_stats.h"
#include "ui_format.h"
#include "oled.h"
#include "font5x8.h"
#include <stdio.h>
#include <string.h>

static uint8_t fb[8][132]; static int pg, col, ack_ok = 1, flipcmds;
void oled_hw_init(void) {}
int oled_hw_write(uint8_t a8, const uint8_t *d, uint16_t n)
{
  if (!ack_ok || a8 != 0x78) return 1;
  if (d[0] == 0x00) {
    for (uint16_t i = 1; i < n; i++) {
      uint8_t c = d[i];
      if (c==0xD5||c==0xA8||c==0xD3||c==0xAD||c==0x8D||c==0x20||c==0xDA||c==0x81||c==0xD9||c==0xDB) { i++; continue; }
      if ((c & 0xF8) == 0xB0) pg = c & 7;
      else if ((c & 0xF0) == 0x00) col = (col & 0xF0) | (c & 0x0F);
      else if ((c & 0xF0) == 0x10) col = (col & 0x0F) | ((c & 0x0F) << 4);
      if (c == 0xA1 || c == 0xC8) flipcmds++;
    }
  } else if (d[0] == 0x40) { for (uint16_t i = 1; i < n; i++) if (col < 132) fb[pg][col++] = d[i]; }
  return 0;
}
static int fails;
#define CHECK(c) do{ if(!(c)){ printf("  FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static void decode(int line, char *out)    /* read characters back out of the framebuffer (SH1106 offset = 2) */
{
  for (int i = 0; i < 21; i++) {
    int found = '?';
    for (int ch = 32; ch < 127; ch++) if (!memcmp(&fb[line][2 + i * 6], font5x8[ch - 32], 5)) { found = ch; break; }
    out[i] = (char)found;
  }
  out[21] = 0;
}
extern void usbi_log_putc(char);
static usbi_setup_t S(uint8_t rt,uint8_t rq,uint16_t v,uint16_t i,uint16_t l){ usbi_setup_t s={rt,rq,v,i,l}; return s; }
int main(void)
{
  /* 1. drive some real traffic through the protocol layer */
  uint8_t w[] = {0x00,0x10,0xAA,0xBB}, a[] = {0x00,0x10}, z[] = {0}, r[8];
  usbi_setup_t s;
  s=S(0x40,USBI_REQ_WRITE,0x34,0,4); usbi_vendor_out(&s,w,4);
  s=S(0x40,USBI_REQ_ADDR ,0x34,0,2); usbi_vendor_out(&s,a,2);
  s=S(0xC0,USBI_REQ_READ ,0x34,0,2); usbi_vendor_in(&s,r,8);
  s=S(0x40,USBI_REQ_WRITE,0x35,0,1); usbi_vendor_out(&s,z,1);          /* absent slave -> NACK */
  s=S(0x40,0xC3,0,0,0);              usbi_vendor_out(&s,NULL,0);        /* unknown request */
  /* 2. format + draw */
  usbi_stats_t st; char L[8][22];
  usbi_stats_get(&st);
  ui_format(L, &st, 3, 754321u);
  CHECK(oled_init() == 1);
  for (int i = 0; i < 8; i++) oled_text_line(i, L[i]);
  /* 3. decode what the "display" shows and compare */
  const char *exp[8] = {
    "USBi 0456:7031    CFG",
    "Bus I2C   Addr 0x35  ",
    "Last WR 00001B  NACK ",
    "Last RD 00002B  OK   ",
    "WR 00003 RD 00001    ",
    "NACK 001 BERR 000    ",
    "C3 v0000 i0000 l0000 ",
    "Unk C3 x01 Up 0:12:34" };
  printf("\n  Screen as the OLED would show it (decoded from simulated pixels):\n  +---------------------+\n");
  for (int i = 0; i < 8; i++) { char got[22]; decode(i, got); printf("  |%s|\n", got); CHECK(strcmp(got, exp[i]) == 0 || (printf("    expected |%s|\n", exp[i]), 0)); }
  printf("  +---------------------+\n");
  CHECK(flipcmds == 2);                       /* orientation commands were sent */
  /* 4. unchanged text must not generate new I2C traffic; changed text must */
  memset(fb, 0xEE, sizeof fb); oled_text_line(1, L[1]); CHECK(fb[1][2] == 0xEE);
  oled_text_line(1, "changed"); CHECK(fb[1][2] != 0xEE);
  /* 5. no display attached -> init reports 0 and drawing is a no-op */
  ack_ok = 0; CHECK(oled_init() == 0); memset(fb, 0x11, sizeof fb); oled_text_line(3, "x"); CHECK(fb[3][2] == 0x11);
  /* 6. state names, SPI line, before-any-traffic screen */
  ack_ok = 1; usbi_stats_t e; memset(&e, 0, sizeof e); e.last_bus = 0xFF;
  ui_format(L, &e, 0, 0); CHECK(!strncmp(L[0], "USBi 0456:7031    ---", 21)); CHECK(!strncmp(L[1], "Bus --", 6)); CHECK(!strncmp(L[6], "Req --", 6)); CHECK(!strncmp(L[7], "Unk -- x00 Up 0:00:00", 21));
  e.last_bus = 1; e.last_cs = 3; e.last_addr = 0x00; ui_format(L, &e, 4, 0); CHECK(!strncmp(L[1], "Bus SPI   CS3 A 0x00", 20)); CHECK(!strncmp(L[0]+18, "SUS", 3));
  printf(fails ? "\n%d FAILURES\n" : "\nOLED/UI: ALL PASSED\n", fails); return fails;
}
