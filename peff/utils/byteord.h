#ifndef _PEFF_UTILS_BYTEORD_H_
#define _PEFF_UTILS_BYTEORD_H_

#include "basedefs.h"
#include <cstdint>

namespace peff {

	PEFF_UTILS_API extern bool _g_byteOrder;

	/// @brief Get current machine byte order.
	/// @return true The machine is in big-endian mode.
	/// @return false The machine is in little-endian mode.
	PEFF_FORCEINLINE bool getByteOrder() {
		return _g_byteOrder;
	}

	constexpr PEFF_FORCEINLINE bool getComptimeByteOrder() {
#if defined(__GNUC__) || defined(__clang__)
		return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
#elif defined(_MSC_VER)
		return 0;
#else
		return getByteOrder();
#endif
	}

	constexpr PEFF_FORCEINLINE uint16_t swapByteOrder(uint16_t n) {
		return (n & 0xff << 8) | (n >> 8);
	}
	constexpr PEFF_FORCEINLINE uint16_t swapByteOrder(int16_t n) {
		return (n & 0xff << 8) | (n >> 8);
	}
	constexpr PEFF_FORCEINLINE uint32_t swapByteOrder(uint32_t n) {
		return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
	}
	constexpr PEFF_FORCEINLINE uint32_t swapByteOrder(int32_t n) {
		return (n & 0xff << 24) | (n & 0xff00 << 8) | (n & 0xff0000 >> 8) | (n >> 24);
	}
	constexpr PEFF_FORCEINLINE uint64_t swapByteOrder(uint64_t n) {
		return ((n & 0xffull) << 56) |
			   ((n & 0xff00ull) << 40) |
			   ((n & 0xff0000ull) << 24) |
			   ((n & 0xff000000ull) << 8) |
			   ((n & 0xff00000000ull) >> 8) |
			   ((n & 0xff0000000000ull) >> 24) |
			   ((n & 0xff000000000000ull) >> 40) |
			   ((n & 0xff00000000000000ull) >> 56);
	}
	constexpr PEFF_FORCEINLINE uint64_t swapByteOrder(int64_t n) {
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
