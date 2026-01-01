#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include "basedefs.h"
#include <type_traits>
#include <functional>
#include <climits>

namespace peff {
	template <uintmax_t Max>
	struct AutoSizeUInteger {
		using type =
			std::conditional_t<
				Max <= UINT8_MAX,
				uint8_t,
				std::conditional_t<
					Max <= UINT16_MAX,
					uint16_t,
					std::conditional_t<
						Max <= UINT32_MAX,
						uint32_t,
						std::conditional_t<
							Max <= UINT64_MAX,
							uint64_t,
							uintmax_t>>>>;
	};
	
	template <typename... Ts>
	struct IsOneOf {
		constexpr static bool value = false;
	};

	template <typename T, typename U, typename... Ts>
	struct IsOneOf<T, U, Ts...> {
		constexpr static bool value =
			std::is_same_v<T, U>
				? true
				: IsOneOf<T, Ts...>::value;
	};
}

#endif
