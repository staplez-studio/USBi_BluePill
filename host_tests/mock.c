#include "usbi_hw.h"
#include <string.h>
#include <stdio.h>
uint8_t mem[65536]; uint8_t spimem[65536];
char wire[4096]; static int wl;
#define LOG(...) (wl += snprintf(wire+wl, sizeof wire-wl, __VA_ARGS__))
/* i2c slave 7-bit 0x34 */
static int phase, rd, sub, nb; static int bus_active;
int usbi_i2c_start(void){ LOG(bus_active?"Sr ":"S "); bus_active=1; phase=0; return 0; }
void usbi_i2c_stop(void){ LOG("P\n"); bus_active=0; }
int usbi_i2c_wr(uint8_t b){
  if(phase==0){ phase=1; LOG("A%02X ",b); if((b>>1)!=0x34) return 3; rd=b&1; if(!rd) nb=0; return 0; }
  LOG("%02X ",b);
  if(nb==0) sub=b<<8; else if(nb==1) sub|=b; else mem[sub++]=b; nb++; return 0; }
int usbi_i2c_rd(uint8_t *b,int ack){ *b=mem[sub++]; LOG("r%02X%s ",*b,ack?"+":"-"); return 0; }
/* spi */
static int cs=-1, sp_nb, sp_rd, sp_sub;
void usbi_spi_cs_assert(uint8_t i){ if(i>=1&&i<=5){cs=i; sp_nb=0; LOG("CS%d ",i);} }
void usbi_spi_cs_release(void){ if(cs!=-1) LOG("CSoff\n"); cs=-1; }
void usbi_spi_wr(uint8_t b){ LOG("s%02X ",b); if(sp_nb==0) sp_rd=b&1; else if(sp_nb==1) sp_sub=b<<8; else if(sp_nb==2) sp_sub|=b; else spimem[sp_sub++]=b; sp_nb++; }
uint8_t usbi_spi_rd(void){ uint8_t v=spimem[sp_sub++]; LOG("m%02X ",v); return v; }
int resets, aux1=-1, aux2=-1;
void usbi_target_reset(void){ resets++; }
void usbi_aux1(int l){ aux1=l; } void usbi_aux2(int l){ aux2=l; } void usbi_led_mode(int s){ (void)s; }
void usbi_hw_init(void){}
void usbi_log_putc(char c){ (void)c; }
void wire_dump(const char*t){ printf("  [%s] %s", t, wire); if(wire[wl?wl-1:0]!='\n') printf("\n"); wl=0; wire[0]=0; }
