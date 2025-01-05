#ifndef _PEFF_CONTAINERS_STRING_H_
#define _PEFF_CONTAINERS_STRING_H_

#include "basedefs.h"
#include <peff/utils/scope_guard.h>
#include <peff/utils/hash.h>
#include <peff/base/allocator.h>
#include "dynarray.h"

namespace peff {
	using String = DynArray<char>;

	template <>
	struct Hasher<String> {
		PEFF_FORCEINLINE uint64_t operator()(const String& x) const {
			return djbHash64(x.data(), x.size());
		}
	};
}

PEFF_CONTAINERS_API bool operator==(const peff::String &lhs, const peff::String &rhs) noexcept;

#endif
