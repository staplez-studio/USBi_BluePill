#include "usbi_stats.h"
volatile usbi_stats_t usbi_stats = { .last_bus = 0xFF };
void usbi_stats_get(usbi_stats_t *dst)
{
  const volatile uint8_t *s = (const volatile uint8_t *)&usbi_stats; uint8_t *d = (uint8_t *)dst;
  for (unsigned i = 0; i < sizeof *dst; i++) d[i] = s[i];
}
