#ifndef _PEFF_CONTAINERS_STRING_H_
#define _PEFF_CONTAINERS_STRING_H_

#include "dynarray.h"

namespace peff {
	template <typename T, typename Allocator = StdAlloc>
	class String final {
	private:
		using DynArray = DynArray<T, Allocator>;
		DynArray _dynArray;
		using ThisType = String<T, Allocator>;

	public:
	};
}

#endif
