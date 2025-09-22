#include "alloc.h"
#include <cstdlib>

using namespace peff;

PEFF_BASE_API Alloc::~Alloc() {}

PEFF_BASE_API StdAlloc peff::g_stdAlloc;

PEFF_BASE_API size_t StdAlloc::incRef(size_t globalRc) noexcept {
	return ++_refCount;
}

PEFF_BASE_API size_t StdAlloc::decRef(size_t globalRc) noexcept {
	if (!--_refCount) {
		onRefZero();
		return 0;
	}
	return _refCount;
}

PEFF_BASE_API void StdAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *StdAlloc::alloc(size_t size, size_t alignment) noexcept {
#ifdef _MSC_VER
	if (alignment <= 1)
		return malloc(size);
	return _aligned_malloc(size, alignment);
#else
	#if __ANDROID__
	return memalign(size, alignment);
	#else
	size_t sizeDiff = size % alignment;
	if (sizeDiff) {
		size += alignment - sizeDiff;
	}
	return aligned_alloc(alignment, size);
	#endif
#endif
}

PEFF_BASE_API void* StdAlloc::realloc(void* ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept {
#ifdef _MSC_VER
	if (alignment > 1) {
		return _aligned_realloc(ptr, newSize, newAlignment);
	} else {
		return ::realloc(ptr, newSize);
	}
#else
	if(alignment > 1) {
		void *p = aligned_alloc(alignment, size);

		if(!p)
			return nullptr;

		memcpy(p, ptr, size);

		free(ptr);

		return p;
	}
	return ::realloc(ptr, newSize);
#endif
}

PEFF_BASE_API void StdAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
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

PEFF_BASE_API bool StdAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID StdAlloc::getTypeId() const noexcept {
	return PEFF_UUID(c8a4e1b0, 4d3a, 4e6c, 8a2f, 6b1c0e9f4d8);
}

PEFF_BASE_API StdAlloc *peff::getDefaultAlloc() noexcept {
	return &g_stdAlloc;
}

PEFF_BASE_API VoidAlloc peff::g_voidAlloc;

PEFF_BASE_API size_t VoidAlloc::incRef(size_t globalRc) noexcept {
	return ++_refCount;
}

PEFF_BASE_API size_t VoidAlloc::decRef(size_t globalRc) noexcept {
	if (!--_refCount) {
		onRefZero();
		return 0;
	}
	return _refCount;
}

PEFF_BASE_API void VoidAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *VoidAlloc::alloc(size_t size, size_t alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API void* VoidAlloc::realloc(void* ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept {
	std::terminate();
}

PEFF_BASE_API void VoidAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API bool VoidAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID VoidAlloc::getTypeId() const noexcept {
	return PEFF_UUID(e3a0c8b2, 1d4e, 4a8f, 8c2d, 6e0f1b9a4c6);
}

PEFF_BASE_API NullAlloc peff::g_nullAlloc;

PEFF_BASE_API size_t NullAlloc::incRef(size_t globalRc) noexcept {
	return ++_refCount;
}

PEFF_BASE_API size_t NullAlloc::decRef(size_t globalRc) noexcept {
	if (!--_refCount) {
		onRefZero();
		return 0;
	}
	return _refCount;
}

PEFF_BASE_API void NullAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *NullAlloc::alloc(size_t size, size_t alignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void* NullAlloc::realloc(void* ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void NullAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
}

PEFF_BASE_API bool NullAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID NullAlloc::getTypeId() const noexcept {
	return PEFF_UUID(a2b8e0c4, 3d1a, 4e8c, 8a2f, 6d0e1b9f4c8);
}
