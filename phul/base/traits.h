#ifndef _PHUL_BASE_TRAITS_
#define _PHUL_BASE_TRAITS_

#include "basedefs.h"
#include <type_traits>

namespace phul {
	template <typename T, typename V = void>
	struct IsCopyable : std::false_type {
	};

	template <typename T>
	struct IsCopyable<T, std::void_t<decltype(std::declval<const T>().copy(*(T*)nullptr))>> : std::true_type {
	};
}

#endif
