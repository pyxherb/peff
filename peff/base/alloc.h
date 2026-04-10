#ifndef _PEFF_BASE_ALLOC_H_
#define _PEFF_BASE_ALLOC_H_

#include "assert.h"
#include "rcobj.h"
#include "scope_guard.h"
#include "uuid.h"
#include <cassert>
#include <memory>

namespace peff {
	class Alloc {
	public:
		PEFF_BASE_API virtual ~Alloc();

		virtual size_t inc_ref(size_t global_ref_count) noexcept = 0;
		virtual size_t dec_ref(size_t global_ref_count) noexcept = 0;

		virtual void *alloc(size_t size, size_t alignment) noexcept = 0;
		virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept = 0;
		virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept = 0;
		virtual void release(void *ptr, size_t size, size_t alignment) noexcept = 0;

		virtual bool is_replaceable(const Alloc *rhs) const noexcept = 0;

		virtual UUID type_identity() const noexcept = 0;
	};

	class StdAlloc : public Alloc {
	private:
		std::atomic_size_t _ref_count = 0;

	public:
		PEFF_BASE_API virtual size_t inc_ref(size_t global_ref_count) noexcept override;
		PEFF_BASE_API virtual size_t dec_ref(size_t global_ref_count) noexcept override;

		PEFF_BASE_API virtual void on_ref_zero() noexcept;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		PEFF_BASE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual UUID type_identity() const noexcept override;
	};

	PEFF_BASE_API extern StdAlloc g_std_allocator;

	PEFF_BASE_API StdAlloc *default_allocator() noexcept;

	class VoidAlloc : public Alloc {
	private:
		std::atomic_size_t _ref_count = 0;

	public:
		PEFF_BASE_API virtual size_t inc_ref(size_t global_ref_count) noexcept override;
		PEFF_BASE_API virtual size_t dec_ref(size_t global_ref_count) noexcept override;

		PEFF_BASE_API virtual void on_ref_zero() noexcept;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_BASE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual UUID type_identity() const noexcept override;
	};

	PEFF_BASE_API extern VoidAlloc g_void_alloc;

	class NullAlloc : public Alloc {
	private:
		std::atomic_size_t _ref_count = 0;

	public:
		PEFF_BASE_API virtual size_t inc_ref(size_t global_ref_count) noexcept override;
		PEFF_BASE_API virtual size_t dec_ref(size_t global_ref_count) noexcept override;

		PEFF_BASE_API virtual void on_ref_zero() noexcept;

		PEFF_BASE_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_BASE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_BASE_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_BASE_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		PEFF_BASE_API virtual UUID type_identity() const noexcept override;
	};

	PEFF_BASE_API extern NullAlloc g_null_alloc;

	PEFF_FORCEINLINE void verify_allocator(const Alloc *x, const Alloc *y) {
		if (x && y) {
			// Check if the allocators have the same type.
			PEFF_ASSERT(x->type_identity() == y->type_identity(), "Incompatible allocators");
		}
	}

	PEFF_FORCEINLINE void verify_replaceable(const Alloc *x, const Alloc *y) {
		if (x && y) {
			// Check if the allocators have the same type.
			PEFF_ASSERT(x->is_replaceable(y), "Incompatible allocators");
		}
	}

#if __cplusplus >= 202002L
	template <typename T, typename... Args>
	requires std::constructible_from<T, Args...>
#else
	template <typename T, typename... Args>
#endif
		PEFF_FORCEINLINE void construct_at(T *ptr, Args &&...args) {
#ifdef new
	#if __cplusplus >= 202002L
		std::construct_at<T>(ptr, std::forward<Args>(args)...);
	#else
		#if defined(_MSC_VER) || (defined(__GNUC__)) || (defined(__clang__))
			#pragma push_macro("new")
			#undef new
		new (ptr) T(std::forward<Args>(args)...);
			#pragma pop_macro("new")
		#else
		std::allocator_traits<std::allocator<T>> allocator;
		allocator.construct(ptr, std::forward<Args>(args)...);
		#endif
	#endif
#else
		new (ptr) T(std::forward<Args>(args)...);
#endif
	}

	template <typename T>
	PEFF_FORCEINLINE void destroy_at(T *const ptr) {
		std::destroy_at<T>(ptr);
	}

#if __cplusplus >= 202002L
	template <typename T, typename... Args>
	requires std::constructible_from<T, Args...>
#else
	template <typename T, typename... Args>
#endif
		PEFF_FORCEINLINE T *alloc_and_construct(Alloc *allocator, size_t alignment, Args &&...args) {
		RcObjectPtr<Alloc> allocator_holder(allocator);

		void *ptr = allocator_holder->alloc(sizeof(T), alignment);
		if (!ptr)
			return nullptr;

		ScopeGuard release_ptr_guard([&allocator_holder, ptr, alignment]() noexcept {
			allocator_holder->release(ptr, sizeof(T), alignment);
		});

		peff::construct_at<T>((T *)ptr, std::forward<Args>(args)...);

		release_ptr_guard.release();

		return (T *)ptr;
	}

	/// @brief Destroy and release an object.
	/// @tparam T The final type of the object
	/// @note T must be the final type of the object, the behavior is undefined if T is not the final type.
	template <typename T>
	PEFF_FORCEINLINE void destroy_and_release(Alloc *allocator, T *ptr, size_t alignment) {
		RcObjectPtr<Alloc> allocator_holder(allocator);
		std::destroy_at<T>(ptr);
		allocator_holder->release((void *)ptr, sizeof(T), alignment);
	}

	template <typename T>
	struct ParamBasedAllocUniquePtrDeleter {
		peff::RcObjectPtr<peff::Alloc> allocator;
		size_t alignment;

		PEFF_FORCEINLINE ParamBasedAllocUniquePtrDeleter(peff::Alloc *allocator, size_t alignment = alignof(T)) : allocator(allocator), alignment(alignment) {
		}

		PEFF_FORCEINLINE void operator()(T *ptr) const {
			if (ptr)
				peff::destroy_and_release<T>(allocator.get(), ptr, alignment);
		}
	};
}

#endif
