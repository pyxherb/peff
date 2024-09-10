#ifndef _PHUL_BASE_TRAITS_
#define _PHUL_BASE_TRAITS_

#include "basedefs.h"

namespace phul {
	template <typename T>
	struct IsCopyable {
	private:
		template <typename U>
		static auto _test(int) -> decltype(std::declval<U>().copy(*(T*)nullptr), std::true_type());
		template<typename U>
		static std::false_type _test(...);

	public:
		static constexpr bool value = std::is_same<decltype(_test<T>(0)), std::true_type>::value;
	};
}

#endif
