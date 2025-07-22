#ifndef _PEFF_BASE_UUID_H_
#define _PEFF_BASE_UUID_H_

#include "basedefs.h"
#include <cstdint>
#include <cstring>

namespace peff {
	struct UUID {
		uint32_t a;
		uint16_t b, c, d;
		uint64_t e;

		PEFF_FORCEINLINE bool operator<(const UUID &other) const {
			return memcmp(this, &other, sizeof(UUID)) < 0;
		}

		PEFF_FORCEINLINE bool operator>(const UUID &other) const {
			return memcmp(this, &other, sizeof(UUID)) > 0;
		}

		PEFF_FORCEINLINE bool operator==(const UUID &other) const {
			return !memcmp((void *)this, (void *)&other, sizeof(UUID));
		}

		PEFF_FORCEINLINE bool operator!=(const UUID &other) const {
			return memcmp((void *)this, (void *)&other, sizeof(UUID));
		}
	};
}

#define PEFF_UUID(a, b, c, d, e) \
	peff::UUID { 0x##a##U, 0x##b##U, 0x##c##U, 0x##d##U, 0x##e##UL }

#endif
