#ifndef _PEFF_BASE_ALLOCATOR_H_
#define _PEFF_BASE_ALLOCATOR_H_

#include "traits.h"
#include "rcobj.h"
#include "scope_guard.h"
#include <cassert>
#include <memory>

namespace peff {
	class Alloc : public RcObject {
	public:
		PEFF_BASE_API virtual ~Alloc();

		virtual void *alloc(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept = 0;
		virtual void release(void *ptr, size_t alignment = alignof(std::max_align_t)) noexcept = 0;

		virtual Alloc *getDefaultAlloc() const noexcept = 0;
	};

	class StdAlloc : public Alloc {
	public:
		PEFF_BASE_API virtual void onRefZero() noexcept override;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t alignment) noexcept override;

		PEFF_BASE_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	extern StdAlloc g_stdAlloc;
	extern RcObjectPtr<StdAlloc> g_stdAllocKeeper;

	PEFF_BASE_API StdAlloc *getDefaultAlloc() noexcept;

	class VoidAlloc : public Alloc {
	public:
		PEFF_BASE_API virtual void onRefZero() noexcept override;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t alignment) noexcept override;

		PEFF_BASE_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	extern VoidAlloc g_voidAlloc;
	extern RcObjectPtr<VoidAlloc> g_voidAllocKeeper;

	PEFF_FORCEINLINE void verifyAlloc(const Alloc *x, const Alloc *y) {
		// Check if the allocators have the same type.
		assert(("Incompatible allocators", x->getDefaultAlloc() == y->getDefaultAlloc()));
	}

	template <typename T, typename... Args>
	PEFF_FORCEINLINE T *allocAndConstruct(Alloc *allocator, size_t alignment, Args... args) {
		RcObjectPtr<Alloc> allocatorHolder(allocator);

		void *ptr = allocatorHolder->alloc(sizeof(T), alignment);
		if (!ptr)
			return nullptr;

		ScopeGuard releasePtrGuard([&allocatorHolder, ptr, alignment]() {
			allocatorHolder->release(ptr, alignment);
		});

		new (ptr) T(std::forward<Args>(args)...);

		releasePtrGuard.release();

		return (T *)ptr;
	}

	template <typename T>
	PEFF_FORCEINLINE void deallocAndDestruct(Alloc *allocator, T *ptr, size_t alignment) {
		RcObjectPtr<Alloc> allocatorHolder(allocator);
		std::destroy_at<T>(ptr);
		allocatorHolder->release((void*)ptr, alignment);
	}
}

#endif
