#include <stdlib.h>
#include <kernel/panic.h>

__attribute__((__noreturn__))
void abort(void) {
	panic("abort()");
}
