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

		PEFF_FORCEINLINE constexpr bool operator<(const UUID &rhs) const {
			if (a < rhs.a)
				return true;
			if (a > rhs.a)
				return false;
			if (b < rhs.b)
				return true;
			if (b > rhs.b)
				return false;
			if (c < rhs.c)
				return true;
			if (c > rhs.c)
				return false;
			if (d < rhs.d)
				return true;
			if (d > rhs.d)
				return false;
			if (e < rhs.e)
				return true;
			if (e > rhs.e)
				return false;
			return false;
		}

		PEFF_FORCEINLINE bool operator>(const UUID &rhs) const {
			return memcmp(this, &rhs, sizeof(UUID)) > 0;
		}

		PEFF_FORCEINLINE bool operator==(const UUID &rhs) const {
			return (a == rhs.a) && (b == rhs.b) && (c == rhs.c) && (d == rhs.d) && (e == rhs.e);
		}

		PEFF_FORCEINLINE bool operator!=(const UUID &rhs) const {
			return (a != rhs.a) || (b != rhs.b) || (c != rhs.c) || (d != rhs.d) || (e != rhs.e);
		}
	};
}

#define PEFF_UUID(a, b, c, d, e) \
	peff::UUID { 0x##a##U, 0x##b##U, 0x##c##U, 0x##d##U, 0x##e##UL }

#endif
