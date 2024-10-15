#ifndef _PHUL_BASE_ALLOCATOR_H_
#define _PHUL_BASE_ALLOCATOR_H_

#include "traits.h"
#include <cassert>

namespace phul {
	class StdAlloc {
	public:
		PHUL_BASE_API void *alloc(size_t size, size_t alignment = alignof(std::max_align_t));
		PHUL_BASE_API void release(void *ptr);

		PHUL_FORCEINLINE bool copy(StdAlloc& dest) const {
			return true;
		}
	};

	class VoidAlloc {
	public:
		PHUL_FORCEINLINE void *alloc(size_t size, size_t alignment = 0) {
			assert(("Cannot allocate memory by VoidAlloc" , false));
		}
		PHUL_FORCEINLINE void release(void *ptr) {
			assert(("Cannot free memory by VoidAlloc", false));
		}

		PHUL_FORCEINLINE bool copy(VoidAlloc &dest) const {
			return true;
		}
	};
}

#endif
