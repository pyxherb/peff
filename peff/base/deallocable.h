#ifndef _PEFF_BASE_DEALLOCABLE_H_
#define _PEFF_BASE_DEALLOCABLE_H_

#include "basedefs.h"
#include <type_traits>

namespace peff {
	template <typename T, typename V = void>
	struct IsDeallocable : std::false_type {
	};

	template <typename T>
	struct IsDeallocable<T, std::void_t<decltype(std::declval<const T>().dealloc())>> : std::true_type {
	};

#if __cplusplus >= 202002L
	template <typename T>
	concept DeallocableConcept = IsDeallocable<T>::value;
#endif

	#if __cplusplus >= 202002L
	template <typename T> requires DeallocableConcept<T>
	#else
	template<typename T>
	#endif
	struct DeallocableDeleter {
		PEFF_FORCEINLINE void operator()(T *ptr) const noexcept {
			if (ptr)
				ptr->dealloc();
		}
	};
}

#endif
