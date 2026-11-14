#include <stdio.h>
#include <kernel/log.h>

int putchar(int ic) {
    klog_write_char((char)ic);
    return ic;
}
