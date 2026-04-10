#include "panic.h"
#include <cstdio>
#include <cstdlib>

using namespace peff;

static void _default_panic(const char *msg) {
	fprintf(stderr, ">>PANIC<<: %s\n", msg);
	abort();
}

static PanicHandler _s_panic_handler = _default_panic;

PEFF_BASE_API void peff::panic(const char *msg) {
	_s_panic_handler(msg);
}
