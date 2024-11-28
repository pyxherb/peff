#ifndef _PEFF_BASE_TRAITS_
#define _PEFF_BASE_TRAITS_

#include "basedefs.h"
#include <utility>
#include <type_traits>

namespace peff {
	template <typename T, typename V = void>
	struct IsCopyable : std::false_type {
	};

	template <typename T>
	struct IsCopyable<T, std::void_t<decltype(std::declval<const T>().copy(*(T*)nullptr))>> : std::true_type {
	};
}

#endif
