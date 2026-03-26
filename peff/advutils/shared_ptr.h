#ifndef _PEFF_ADVUTILS_SHARED_PTR_H_
#define _PEFF_ADVUTILS_SHARED_PTR_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <memory>
#include <atomic>

namespace peff {
	struct SharedPtrControlBlock {
		std::atomic_size_t weak_ref_num = 0, strong_ref_num = 0;

		PEFF_FORCEINLINE SharedPtrControlBlock() {}
		inline virtual ~SharedPtrControlBlock() {}

		PEFF_FORCEINLINE void inc_strong_ref() noexcept {
			++strong_ref_num;
		}

		PEFF_FORCEINLINE void inc_weak_ref() noexcept {
			++weak_ref_num;
		}

		PEFF_FORCEINLINE void dec_strong_ref() noexcept {
			if (!--strong_ref_num) {
				if (weak_ref_num) {
					on_strong_ref_zero();
				} else {
					on_strong_ref_zero();
					on_ref_zero();
				}
			}
		}

		PEFF_FORCEINLINE void dec_weak_ref() noexcept {
			if (!--weak_ref_num) {
				if (!strong_ref_num) {
					on_ref_zero();
				}
			}
		}

		virtual void on_strong_ref_zero() noexcept = 0;
		virtual void on_ref_zero() noexcept = 0;
	};

	class SharedFromThisBase {
	protected:
		SharedPtrControlBlock *control_block;

		friend class _SharedFromThisHelper;
	};

	class _SharedFromThisHelper {
	public:
		PEFF_FORCEINLINE static void set_control_block(SharedFromThisBase *sft, SharedPtrControlBlock *control_block) noexcept {
			sft->control_block = control_block;
		}
	};

	template <typename T>
	class WeakPtr;

	template <typename T>
	class SharedPtr {
	public:
		struct DefaultSharedPtrControlBlock : public SharedPtrControlBlock {
			T *ptr;
			peff::RcObjectPtr<peff::Alloc> allocator;

			PEFF_FORCEINLINE DefaultSharedPtrControlBlock(peff::Alloc *allocator, T *ptr) noexcept : allocator(allocator), ptr(ptr) {}
			inline virtual ~DefaultSharedPtrControlBlock() {}

			inline virtual void on_strong_ref_zero() noexcept override {
				peff::destroy_and_release<T>(allocator.get(), ptr, alignof(T));
			}

			inline virtual void on_ref_zero() noexcept override {
				peff::destroy_and_release<DefaultSharedPtrControlBlock>(allocator.get(), this, alignof(DefaultSharedPtrControlBlock));
			}
		};

		class SharedFromThis : public SharedFromThisBase {
		protected:
			friend class SharedPtr<T>;

		public:
			PEFF_FORCEINLINE SharedPtr<T> shared_from_this() noexcept {
				return SharedPtr<T>(control_block, static_cast<T *>(this));
			}
		};

	private:
		SharedPtrControlBlock *control_block;
		T *ptr;

		friend class WeakPtr<T>;
		friend class SharedFromThis;

	public:
		PEFF_FORCEINLINE void reset() noexcept {
			if (control_block)
				control_block->dec_strong_ref();
			control_block = nullptr;
			ptr = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr() noexcept : control_block(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtrControlBlock *control_block, T *ptr) noexcept : control_block(control_block), ptr(ptr) {
			if (control_block) {
				control_block->inc_strong_ref();
			}

			if constexpr (std::is_base_of_v<SharedFromThisBase, T>) {
				_SharedFromThisHelper::set_control_block(static_cast<SharedFromThisBase *>(ptr), control_block);
			}
		}
		PEFF_FORCEINLINE ~SharedPtr() {
			reset();
		}

		PEFF_FORCEINLINE SharedPtr(const SharedPtr<T> &rhs) noexcept : control_block(rhs.control_block), ptr(rhs.ptr) {
			if (control_block) {
				control_block->inc_strong_ref();
			}
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtr<T> &&rhs) noexcept : control_block(rhs.control_block), ptr(rhs.ptr) {
			rhs.control_block = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr<T> &operator=(const SharedPtr<T> &rhs) noexcept {
			if (this == &rhs)
				return *this;
			reset();
			control_block = rhs.control_block;
			if (control_block) {
				control_block->inc_strong_ref();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE SharedPtr<T> &operator=(SharedPtr<T> &&rhs) noexcept {
			if (this == &rhs)
				return *this;
			reset();
			control_block = rhs.control_block;
			ptr = rhs.ptr;
			rhs.control_block = nullptr;
			rhs.ptr = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE T *get() const noexcept {
			return (T *)ptr;
		}

		PEFF_FORCEINLINE T *operator->() const noexcept {
			return get();
		}
		PEFF_FORCEINLINE T &operator*() const noexcept {
			return *get();
		}

		PEFF_FORCEINLINE bool operator<(const SharedPtr<T> &rhs) const noexcept {
			return control_block < rhs.control_block;
		}

		PEFF_FORCEINLINE bool operator>(const SharedPtr<T> &rhs) const noexcept {
			return control_block > rhs.control_block;
		}

		PEFF_FORCEINLINE bool operator==(const SharedPtr<T> &rhs) const noexcept {
			return control_block == rhs.control_block;
		}

		PEFF_FORCEINLINE bool operator!=(const SharedPtr<T> &rhs) const noexcept {
			return control_block != rhs.control_block;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return (bool)ptr;
		}

		template <typename T1>
		PEFF_FORCEINLINE SharedPtr<T1> cast_to() const noexcept {
			if ((!control_block) || (!ptr))
				return {};
			return SharedPtr<T1>(control_block, static_cast<T1 *>(ptr));
		}
	};

	template <typename T>
	class WeakPtr {
	public:
		SharedPtrControlBlock *control_block;
		T *ptr;

		PEFF_FORCEINLINE void reset() {
			if (control_block)
				control_block->dec_weak_ref();
			control_block = nullptr;
			ptr = nullptr;
		}

		PEFF_FORCEINLINE WeakPtr() noexcept : control_block(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE explicit WeakPtr(SharedPtrControlBlock *control_block, T *ptr) noexcept : control_block(control_block), ptr(ptr) {
			if (control_block) {
				control_block->inc_weak_ref();
			}
		}
		PEFF_FORCEINLINE WeakPtr(const SharedPtr<T> &ptr) noexcept : control_block(ptr.control_block), ptr(ptr.ptr) {
			if (control_block) {
				control_block->inc_weak_ref();
			}
		}
		PEFF_FORCEINLINE ~WeakPtr() {
			reset();
		}

		PEFF_FORCEINLINE WeakPtr(const WeakPtr<T> &rhs) noexcept : control_block(rhs.control_block), ptr(rhs.ptr) {
			if (control_block) {
				control_block->inc_weak_ref();
			}
		}
		PEFF_FORCEINLINE WeakPtr(WeakPtr<T> &&rhs) noexcept : control_block(rhs.control_block), ptr(rhs.ptr) {
			rhs.control_block = nullptr;
			rhs.ptr = nullptr;
		}

		PEFF_FORCEINLINE WeakPtr<T> &operator=(const WeakPtr<T> &rhs) noexcept {
			reset();
			control_block = rhs.control_block;
			if (control_block) {
				control_block->inc_weak_ref();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE WeakPtr<T> &operator=(WeakPtr<T> &&rhs) noexcept {
			reset();
			control_block = rhs.control_block;
			ptr = rhs.ptr;
			rhs.control_block = nullptr;
			rhs.ptr = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE bool operator<(const WeakPtr<T> &rhs) const noexcept {
			return control_block < rhs.control_block;
		}

		PEFF_FORCEINLINE bool operator==(const WeakPtr<T> &rhs) const noexcept {
			return control_block == rhs.control_block;
		}

		PEFF_FORCEINLINE bool operator!=(const WeakPtr<T> &rhs) const noexcept {
			return control_block != rhs.control_block;
		}

		PEFF_FORCEINLINE SharedPtr<T> lock() const noexcept {
			if ((!control_block) || (!control_block->strong_ref_num))
				std::terminate();
			return SharedPtr<T>(control_block, ptr);
		}

		template <typename T1>
		PEFF_FORCEINLINE WeakPtr<T1> cast_to() const noexcept {
			return WeakPtr<T1>(control_block, ptr + (((char *)static_cast<T *>(ptr)) - (char *)static_cast<T1 *>(ptr)));
		}

		PEFF_FORCEINLINE bool is_valid() const noexcept {
			if (!control_block)
				return false;
			return control_block->strong_ref_num;
		}
	};

	template <typename T>
	using SharedFromThis = typename SharedPtr<T>::SharedFromThis;

	template <typename T, typename... Args>
	PEFF_FORCEINLINE SharedPtr<T> make_shared(peff::Alloc *allocator, Args &&...args) {
		T *ptr = peff::alloc_and_construct<T>(allocator, alignof(T), std::forward<Args>(args)...);
		if (!ptr)
			return {};
		peff::ScopeGuard release_ptr_guard([allocator, ptr]() noexcept {
			peff::destroy_and_release<T>(allocator, ptr, alignof(T));
		});

		typename SharedPtr<T>::DefaultSharedPtrControlBlock *control_block =
			peff::alloc_and_construct<typename SharedPtr<T>::DefaultSharedPtrControlBlock>(
				allocator, alignof(typename SharedPtr<T>::DefaultSharedPtrControlBlock),
				allocator, ptr);
		if (!control_block)
			return {};
		release_ptr_guard.release();

		SharedPtr<T> p(control_block, ptr);

		return p;
	}

	template <typename T, typename D, typename... Args>
	PEFF_FORCEINLINE SharedPtr<T> make_shared_with_control_block(peff::Alloc *allocator, Args &&...args) {
		T *ptr = peff::alloc_and_construct<T>(allocator, alignof(T), std::forward<Args>(args)...);
		if (!ptr)
			return {};
		peff::ScopeGuard release_ptr_guard([allocator, ptr]() noexcept {
			peff::destroy_and_release<T>(allocator, ptr, alignof(T));
		});

		D *control_block =
			peff::alloc_and_construct<D>(
				allocator, alignof(D),
				allocator, ptr);
		if (!control_block)
			return {};
		release_ptr_guard.release();

		SharedPtr<T> p(control_block, ptr);

		return p;
	}
}

#endif
