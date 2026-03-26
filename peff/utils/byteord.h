#ifndef _PEFF_UTILS_BYTEORD_H_
#define _PEFF_UTILS_BYTEORD_H_

#include "basedefs.h"
#include <cstdint>

namespace peff {
	PEFF_UTILS_API extern bool _g_byte_order;

	/// @brief Get current machine byte order.
	/// @return true The machine is in big-endian mode.
	/// @return false The machine is in little-endian mode.
	PEFF_FORCEINLINE bool get_byte_order() {
		return _g_byte_order;
	}

	constexpr PEFF_FORCEINLINE bool get_comptime_byte_order() {
#if defined(__GNUC__) || defined(__clang__)
		return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
#elif defined(_MSC_VER)
		return 0;
#else
		return get_byte_order();
#endif
	}

	constexpr PEFF_FORCEINLINE uint16_t swap_byte_order(uint16_t n) {
		return (n & 0xff << 8) | (n >> 8);
	}
	constexpr PEFF_FORCEINLINE uint16_t swap_byte_order(int16_t n) {
		return (n & 0xff << 8) | (n >> 8);
	}
	constexpr PEFF_FORCEINLINE uint32_t swap_byte_order(uint32_t n) {
		return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
	}
	constexpr PEFF_FORCEINLINE uint32_t swap_byte_order(int32_t n) {
		return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
	}
	constexpr PEFF_FORCEINLINE uint64_t swap_byte_order(uint64_t n) {
		return ((n & 0xffull) << 56) |
			   ((n & 0xff00ull) << 40) |
			   ((n & 0xff0000ull) << 24) |
			   ((n & 0xff000000ull) << 8) |
			   ((n & 0xff00000000ull) >> 8) |
			   ((n & 0xff0000000000ull) >> 24) |
			   ((n & 0xff000000000000ull) >> 40) |
			   ((n & 0xff00000000000000ull) >> 56);
	}
	constexpr PEFF_FORCEINLINE uint64_t swap_byte_order(int64_t n) {
		return ((n & 0xffull) << 56) |
			   ((n & 0xff00ull) << 40) |
			   ((n & 0xff0000ull) << 24) |
			   ((n & 0xff000000ull) << 8) |
			   ((n & 0xff00000000ull) >> 8) |
			   ((n & 0xff0000000000ull) >> 24) |
			   ((n & 0xff000000000000ull) >> 40) |
			   ((n & 0xff00000000000000ull) >> 56);
	}
}

#endif
