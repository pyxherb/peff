#include "allocator.h"
#include <cstdlib>

using namespace peff;

PEFF_BASE_API void *StdAlloc::alloc(size_t size, size_t alignment) {
#ifdef _MSC_VER
	return _aligned_malloc(size, alignment);
#else
	return aligned_alloc(size, alignment);
#endif
}

PEFF_BASE_API void StdAlloc::release(void *ptr) {
#ifdef _MSC_VER
	return _aligned_free(ptr);
#else
	return free(ptr);
#endif
}
