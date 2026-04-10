#ifndef _PEFF_BASE_PANIC_H_
#define _PEFF_BASE_PANIC_H_

#include "basedefs.h"

namespace peff {
	typedef void (*PanicHandler)(const char *msg);

	PEFF_BASE_API void panic(const char *msg);
}

#endif
