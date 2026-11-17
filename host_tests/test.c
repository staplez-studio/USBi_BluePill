#include "usbi_proto.h"
#include <stdio.h>
#include <string.h>
extern uint8_t mem[], spimem[]; extern int resets, aux1, aux2; void wire_dump(const char*);
static int fails;
#define CHECK(c) do{ if(!(c)){ printf("  FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static usbi_setup_t S(uint8_t rt,uint8_t rq,uint16_t v,uint16_t i,uint16_t l){ usbi_setup_t s={rt,rq,v,i,l}; return s; }
static uint8_t status(uint8_t rq){ uint8_t b[4]; usbi_setup_t s=S(0xC0,rq,0,0,1); CHECK(usbi_vendor_in(&s,b,sizeof b)==1); return b[0]; }
int main(void){
  printf("1) I2C write: B2 wValue=0x34 data=[00 10 AA BB]\n");
  usbi_setup_t s=S(0x40,USBI_REQ_WRITE,0x34,0,4); uint8_t d[]={0x00,0x10,0xAA,0xBB};
  CHECK(usbi_vendor_out(&s,d,4)==0); wire_dump("wire");
  CHECK(mem[0x10]==0xAA && mem[0x11]==0xBB); CHECK(status(USBI_REQ_WR_STATUS)==USBI_ST_OK); CHECK(status(USBI_REQ_WR_STATUS)==0);

  printf("2) I2C register read: B3 [00 10] then B4 len 2, then B6\n");
  s=S(0x40,USBI_REQ_ADDR,0x34,0,2); uint8_t a[]={0x00,0x10}; CHECK(usbi_vendor_out(&s,a,2)==0);
  CHECK(status(USBI_REQ_WR_STATUS)==USBI_ST_OK);
  uint8_t r[8]; s=S(0xC0,USBI_REQ_READ,0x34,0,2); CHECK(usbi_vendor_in(&s,r,8)==2); wire_dump("wire");
  CHECK(r[0]==0xAA && r[1]==0xBB); CHECK(status(USBI_REQ_RD_STATUS)==USBI_ST_OK);

  printf("3) NACK from absent slave 0x35\n");
  s=S(0x40,USBI_REQ_WRITE,0x35,0,1); uint8_t z[]={0}; CHECK(usbi_vendor_out(&s,z,1)==0); wire_dump("wire");
  CHECK(status(USBI_REQ_WR_STATUS)==USBI_ST_NACK);

  printf("4) zero-length B2 (address probe)\n");
  s=S(0x40,USBI_REQ_WRITE,0x34,0,0); CHECK(usbi_vendor_out(&s,NULL,0)==0); wire_dump("wire"); CHECK(status(USBI_REQ_WR_STATUS)==USBI_ST_OK);

  printf("5) SPI write, CS3: B2 wIndex=0x0301 wValue=0 data=[00 20 11 22]\n");
  s=S(0x40,USBI_REQ_WRITE,0x00,0x0301,4); uint8_t sd[]={0x00,0x20,0x11,0x22}; CHECK(usbi_vendor_out(&s,sd,4)==0); wire_dump("wire");
  CHECK(spimem[0x20]==0x11 && spimem[0x21]==0x22);

  printf("6) SPI read: B3 [00 20] (first byte has R bit) then B4 len 2\n");
  s=S(0x40,USBI_REQ_ADDR,0x00,0x0301,2); uint8_t sa[]={0x00,0x20}; CHECK(usbi_vendor_out(&s,sa,2)==0);
  s=S(0xC0,USBI_REQ_READ,0x00,0x0301,2); CHECK(usbi_vendor_in(&s,r,8)==2); wire_dump("wire");
  CHECK(r[0]==0x11 && r[1]==0x22); CHECK(status(USBI_REQ_RD_STATUS)==USBI_ST_OK);

  printf("7) misc: B7 reset, B8 wValue=1, B9 wValue=1 / 0, B1, unknown\n");
  s=S(0x40,USBI_REQ_RESET,0,0,0); CHECK(usbi_vendor_out(&s,NULL,0)==0); CHECK(resets==1);
  s=S(0x40,USBI_REQ_AUX1,1,0,0); usbi_vendor_out(&s,NULL,0); CHECK(aux1==1);
  s=S(0x40,USBI_REQ_AUX2,1,0,0); usbi_vendor_out(&s,NULL,0); CHECK(aux2==0);
  s=S(0x40,USBI_REQ_AUX2,0,0,0); usbi_vendor_out(&s,NULL,0); CHECK(aux2==1);
  CHECK(status(USBI_REQ_B1)==0);
  s=S(0x40,0xC3,0,0,0); CHECK(usbi_vendor_out(&s,NULL,0)<0);
  printf(fails?"\n%d FAILURES\n":"\nALL PASSED\n",fails); return fails;
}
