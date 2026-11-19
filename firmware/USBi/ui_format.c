#include "ui_format.h"
#include <string.h>

static int ps(char *l, int p, const char *s) { while (*s && p < 21) l[p++] = *s++; return p; }
static int pd(char *l, int p, uint32_t v, int w)            /* zero-padded decimal, width w (value wraps) */
{
  char t[10]; for (int i = w - 1; i >= 0; i--) { t[i] = (char)('0' + v % 10); v /= 10; }
  for (int i = 0; i < w && p < 21; i++) l[p++] = t[i];
  return p;
}
static int px(char *l, int p, uint32_t v, int w)            /* upper-case hex, w digits */
{
  static const char h[] = "0123456789ABCDEF";
  for (int i = w - 1; i >= 0 && p < 21; i--) l[p++] = h[(v >> (4 * i)) & 15];
  return p;
}
static const char *st_txt(uint8_t st)
{ return st == 1 ? "OK" : st == 3 ? "NACK" : st == 4 ? "BERR" : "--"; }
static void blank(char *l) { memset(l, ' ', 21); l[21] = 0; }

void ui_format(char L[8][22], const usbi_stats_t *s, uint8_t usb, uint32_t ms)
{
  static const char *const state[5] = { "---", "DEF", "ADR", "CFG", "SUS" };
  int p;
  for (int i = 0; i < 8; i++) blank(L[i]);

  ps(L[0], 0, "USBi 0456:7031"); ps(L[0], 18, state[usb > 4 ? 0 : usb]);

  if (s->last_bus == 0xFF) ps(L[1], 0, "Bus --");
  else if (s->last_bus == 0) { p = ps(L[1], 0, "Bus I2C   Addr 0x"); px(L[1], p, s->last_addr, 2); }
  else { p = ps(L[1], 0, "Bus SPI   CS"); p = pd(L[1], p, s->last_cs, 1); p = ps(L[1], p, " A 0x"); px(L[1], p, s->last_addr, 2); }

  p = ps(L[2], 0, "Last WR "); p = pd(L[2], p, s->last_wr_len, 5); p = ps(L[2], p, "B  "); ps(L[2], p, st_txt(s->last_wr_st));
  p = ps(L[3], 0, "Last RD "); p = pd(L[3], p, s->last_rd_len, 5); p = ps(L[3], p, "B  "); ps(L[3], p, st_txt(s->last_rd_st));

  p = ps(L[4], 0, "WR "); p = pd(L[4], p, s->wr_cnt, 5); p = ps(L[4], p, " RD "); pd(L[4], p, s->rd_cnt, 5);
  p = ps(L[5], 0, "NACK "); p = pd(L[5], p, s->nack_cnt, 3); p = ps(L[5], p, " BERR "); pd(L[5], p, s->berr_cnt, 3);

  if (!s->have_req) ps(L[6], 0, "Req --");
  else {
    p = px(L[6], 0, s->last_rq, 2); p = ps(L[6], p, " v"); p = px(L[6], p, s->last_v, 4);
    p = ps(L[6], p, " i"); p = px(L[6], p, s->last_i, 4); p = ps(L[6], p, " l"); px(L[6], p, s->last_l, 4);
  }

  p = ps(L[7], 0, "Unk ");
  if (s->have_unk) p = px(L[7], p, s->unk_rq, 2); else p = ps(L[7], p, "--");
  p = ps(L[7], p, " x"); p = pd(L[7], p, s->unk_cnt, 2); p = ps(L[7], p, " Up ");
  { uint32_t sec = ms / 1000, m = (sec / 60) % 60, h = (sec / 3600) % 10;
    p = pd(L[7], p, h, 1); p = ps(L[7], p, ":"); p = pd(L[7], p, m, 2); p = ps(L[7], p, ":"); pd(L[7], p, sec % 60, 2); }
}
