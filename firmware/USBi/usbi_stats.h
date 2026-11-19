/* usbi_stats.h - counters and "last transaction" info, updated by the protocol layer (USB interrupt)
 * and read by the OLED status screen (main loop). Reads may tear by one frame; that is harmless. */
#pragma once
#include <stdint.h>
typedef struct {
  uint32_t req_cnt, wr_cnt, rd_cnt, nack_cnt, berr_cnt, unk_cnt;
  uint8_t  last_bus;                 /* 0 = I2C, 1 = SPI, 0xFF = none yet */
  uint8_t  last_addr, last_cs;       /* wValueL, wIndexH */
  uint8_t  last_wr_st, last_rd_st;   /* 0 = none, 1 OK, 3 NACK, 4 bus error */
  uint16_t last_wr_len, last_rd_len;
  uint8_t  last_rq;  uint16_t last_v, last_i, last_l;   /* last request, raw */
  uint8_t  unk_rq;                   /* last unhandled request code */
  uint8_t  have_req, have_unk;
} usbi_stats_t;
extern volatile usbi_stats_t usbi_stats;
void usbi_stats_get(usbi_stats_t *dst);   /* copy for the UI */
