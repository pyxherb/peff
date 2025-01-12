#include "allocator.h"
#include <cstdlib>

using namespace peff;

StdAlloc peff::g_stdAlloc;
RcObjectPtr<StdAlloc> peff::g_stdAllocKeeper(&g_stdAlloc);

PEFF_BASE_API Alloc::~Alloc() {}

PEFF_BASE_API void StdAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *StdAlloc::alloc(size_t size, size_t alignment) noexcept {
#ifdef _MSC_VER
	if (alignment <= 1)
		return malloc(size);
	return _aligned_malloc(size, alignment);
#else
	size_t sizeDiff = size % alignment;
	if (sizeDiff) {
		size += alignment - sizeDiff;
	}
	return aligned_alloc(alignment, size);
#endif
}

PEFF_BASE_API void StdAlloc::release(void *ptr, size_t alignment) noexcept {
#ifdef _MSC_VER
	if (alignment > 1) {
		_aligned_free(ptr);
	} else {
		free(ptr);
	}
#else
	free(ptr);
#endif
}

PEFF_BASE_API Alloc *StdAlloc::getDefaultAlloc() const noexcept {
	return g_stdAllocKeeper.get();
}

PEFF_BASE_API StdAlloc* peff::getDefaultAlloc() noexcept {
	return &g_stdAlloc;
}

VoidAlloc peff::g_voidAlloc;
RcObjectPtr<VoidAlloc> peff::g_voidAllocKeeper(&g_voidAlloc);

PEFF_BASE_API void VoidAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *VoidAlloc::alloc(size_t size, size_t alignment) noexcept {
	assert(("Cannot allocate memory by VoidAlloc", false));
	return nullptr;
}

PEFF_BASE_API void VoidAlloc::release(void *ptr, size_t alignment) noexcept {
	assert(("Cannot free memory by VoidAlloc", false));
}

PEFF_BASE_API Alloc *VoidAlloc::getDefaultAlloc() const noexcept {
	return g_voidAllocKeeper.get();
}
