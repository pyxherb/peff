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

		virtual void *alloc(size_t size, size_t alignment) noexcept = 0;
		virtual void release(void *ptr, size_t size, size_t alignment) noexcept = 0;

		virtual bool isReplaceable(const Alloc *rhs) const noexcept = 0;

		virtual Alloc *getDefaultAlloc() const noexcept = 0;
	};

	class StdAlloc : public Alloc {
	public:
		PEFF_BASE_API virtual void onRefZero() noexcept override;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool isReplaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	PEFF_BASE_API extern StdAlloc g_stdAlloc;
	PEFF_BASE_API extern RcObjectPtr<StdAlloc> g_stdAllocKeeper;

	PEFF_BASE_API StdAlloc *getDefaultAlloc() noexcept;

	class VoidAlloc : public Alloc {
	public:
		PEFF_BASE_API virtual void onRefZero() noexcept override;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool isReplaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	PEFF_BASE_API extern VoidAlloc g_voidAlloc;
	PEFF_BASE_API extern RcObjectPtr<VoidAlloc> g_voidAllocKeeper;

	class NullAlloc : public Alloc {
	public:
		PEFF_BASE_API virtual void onRefZero() noexcept override;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool isReplaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	PEFF_BASE_API extern NullAlloc g_nullAlloc;
	PEFF_BASE_API extern RcObjectPtr<NullAlloc> g_nullAllocKeeper;

	PEFF_FORCEINLINE void verifyAlloc(const Alloc *x, const Alloc *y) {
		if (x && y) {
			// Check if the allocators have the same type.
			assert(("Incompatible allocators", x->getDefaultAlloc() == y->getDefaultAlloc()));
		}
	}

	PEFF_FORCEINLINE void verifyReplaceable(const Alloc *x, const Alloc *y) {
		if (x && y) {
			// Check if the allocators have the same type.
			assert(("Incompatible allocators", x->isReplaceable(y)));
		}
	}

	template <typename T, typename... Args>
	PEFF_FORCEINLINE void constructAt(T *ptr, Args &&...args) {
#ifdef new
	#ifdef _MSC_VER
		#pragma push_macro("new")
		#undef new
		new (ptr) T(std::forward<Args>(args)...);
		#pragma pop_macro("new")
	#else
		std::allocator<T> allocator;
		allocator.construct(ptr, std::forward<Args>(args)...);
	#endif
#else
		new (ptr) T(std::forward<Args>(args)...);
#endif
	}

	template <typename T, typename... Args>
	PEFF_FORCEINLINE T *allocAndConstruct(Alloc *allocator, size_t alignment, Args &&...args) {
		RcObjectPtr<Alloc> allocatorHolder(allocator);

		void *ptr = allocatorHolder->alloc(sizeof(T), alignment);
		if (!ptr)
			return nullptr;

		ScopeGuard releasePtrGuard([&allocatorHolder, ptr, alignment]() noexcept {
			allocatorHolder->release(ptr, sizeof(T), alignment);
		});

		constructAt<T>((T *)ptr, std::forward<Args>(args)...);

		releasePtrGuard.release();

		return (T *)ptr;
	}

	/// @brief Destroy and release an object.
	/// @tparam T The final type of the object
	/// @note T must be the final type of the object, the behavior is undefined if T is not the final type.
	template <typename T>
	PEFF_FORCEINLINE void destroyAndRelease(Alloc *allocator, T *ptr, size_t alignment) {
		RcObjectPtr<Alloc> allocatorHolder(allocator);
		std::destroy_at<T>(ptr);
		allocatorHolder->release((void *)ptr, sizeof(T), alignment);
	}

	template <typename T>
	struct ParamBasedAllocUniquePtrDeleter {
		peff::RcObjectPtr<peff::Alloc> allocator;
		size_t alignment;

		PEFF_FORCEINLINE ParamBasedAllocUniquePtrDeleter(peff::Alloc *allocator, size_t alignment = alignof(T)) : allocator(allocator), alignment(alignment) {
		}

		PEFF_FORCEINLINE void operator()(T *ptr) const {
			if (ptr)
				peff::destroyAndRelease<T>(allocator.get(), ptr, alignment);
		}
	};
}

#endif
