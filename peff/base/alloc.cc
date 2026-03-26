#include "alloc.h"
#include <cstdlib>

using namespace peff;

PEFF_BASE_API Alloc::~Alloc() {}

PEFF_BASE_API StdAlloc peff::g_std_allocator;

PEFF_BASE_API size_t StdAlloc::inc_ref(size_t global_ref_count) noexcept {
	return ++_ref_count;
}

PEFF_BASE_API size_t StdAlloc::dec_ref(size_t global_ref_count) noexcept {
	if (!--_ref_count) {
		on_ref_zero();
		return 0;
	}
	return _ref_count;
}

PEFF_BASE_API void StdAlloc::on_ref_zero() noexcept {
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
	size_t size_diff = size % alignment;
	if (size_diff) {
		size += alignment - size_diff;
	}
	return aligned_alloc(alignment, size);
	#endif
#endif
}

PEFF_BASE_API void *StdAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void *StdAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
#ifdef _MSC_VER
	if (alignment > 1) {
		return _aligned_realloc(ptr, new_size, new_alignment);
	} else {
		return ::realloc(ptr, new_size);
	}
#else
	if (alignment > 1) {
		void *p;

	#if __ANDROID__
		if (!(p = memalign(new_alignment, new_size)))
			return nullptr;
	#else
		if (!(p = aligned_alloc(new_alignment, new_size)))
			return nullptr;
	#endif

		memcpy(p, ptr, size > new_size ? new_size : size);

		free(ptr);

		return p;
	}
	return ::realloc(ptr, new_size);
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

PEFF_BASE_API bool StdAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID StdAlloc::type_identity() const noexcept {
	return PEFF_UUID(c8a4e1b0, 4d3a, 4e6c, 8a2f, 6b1c0e9f4d8);
}

PEFF_BASE_API StdAlloc *peff::default_allocator() noexcept {
	return &g_std_allocator;
}

PEFF_BASE_API VoidAlloc peff::g_void_alloc;

PEFF_BASE_API size_t VoidAlloc::inc_ref(size_t global_ref_count) noexcept {
	return ++_ref_count;
}

PEFF_BASE_API size_t VoidAlloc::dec_ref(size_t global_ref_count) noexcept {
	if (!--_ref_count) {
		on_ref_zero();
		return 0;
	}
	return _ref_count;
}

PEFF_BASE_API void VoidAlloc::on_ref_zero() noexcept {
}

PEFF_BASE_API void *VoidAlloc::alloc(size_t size, size_t alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API void *VoidAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API void *VoidAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API void VoidAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	std::terminate();
}

PEFF_BASE_API bool VoidAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID VoidAlloc::type_identity() const noexcept {
	return PEFF_UUID(e3a0c8b2, 1d4e, 4a8f, 8c2d, 6e0f1b9a4c6);
}

PEFF_BASE_API NullAlloc peff::g_null_alloc;

PEFF_BASE_API size_t NullAlloc::inc_ref(size_t global_ref_count) noexcept {
	return ++_ref_count;
}

PEFF_BASE_API size_t NullAlloc::dec_ref(size_t global_ref_count) noexcept {
	if (!--_ref_count) {
		on_ref_zero();
		return 0;
	}
	return _ref_count;
}

PEFF_BASE_API void NullAlloc::on_ref_zero() noexcept {
}

PEFF_BASE_API void *NullAlloc::alloc(size_t size, size_t alignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void *NullAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void *NullAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	return nullptr;
}

PEFF_BASE_API void NullAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
}

PEFF_BASE_API bool NullAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	return true;
}

PEFF_BASE_API UUID NullAlloc::type_identity() const noexcept {
	return PEFF_UUID(a2b8e0c4, 3d1a, 4e8c, 8a2f, 6d0e1b9f4c8);
}
