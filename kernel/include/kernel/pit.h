#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_HZ 100

#define PIT_COMMAND     0x43
#define PIT_BASE_HZ     1193182

void pit_init(void);
uint64_t pit_ticks(void);

#endif
