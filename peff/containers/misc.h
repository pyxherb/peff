#ifndef _PEFF_CONTAINERS_MISC_H_
#define _PEFF_CONTAINERS_MISC_H_

#include "basedefs.h"
#include <cstdint>

namespace peff {
	enum class IteratorDirection : uint8_t {
		Forward = 0,
		Reversed,
		Invalid
	};
}

#endif
