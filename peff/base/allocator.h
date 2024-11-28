#ifndef _PEFF_BASE_ALLOCATOR_H_
#define _PEFF_BASE_ALLOCATOR_H_

#include "traits.h"
#include <cassert>

namespace peff {
	class StdAlloc {
	public:
		PEFF_BASE_API void *alloc(size_t size, size_t alignment = alignof(std::max_align_t));
		PEFF_BASE_API void release(void *ptr);

		PEFF_FORCEINLINE bool copy(StdAlloc& dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(StdAlloc &dest) const {
			return true;
		}
	};

	class VoidAlloc {
	public:
		PEFF_FORCEINLINE void *alloc(size_t size, size_t alignment = 0) {
			assert(("Cannot allocate memory by VoidAlloc" , false));
		}
		PEFF_FORCEINLINE void release(void *ptr) {
			assert(("Cannot free memory by VoidAlloc", false));
		}

		PEFF_FORCEINLINE bool copy(VoidAlloc &dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(VoidAlloc &dest) const {
			return true;
		}
	};
}

#endif
