#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_HZ 100

void pit_init(void);
uint64_t pit_ticks(void);

#endif
