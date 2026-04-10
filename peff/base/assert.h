#ifndef _PEFF_BASE_ASSERT_H_
#define _PEFF_BASE_ASSERT_H_

#include "panic.h"

#if PEFF_ENABLE_ASSERTION
	#define PEFF_ASSERT(x, message) if ((x) == false) peff::panic(message);
#else
	#define PEFF_ASSERT(x, message)
#endif

#endif
