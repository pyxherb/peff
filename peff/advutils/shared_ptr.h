#ifndef _PEFF_ADVUTILS_SHARED_PTR_H_
#define _PEFF_ADVUTILS_SHARED_PTR_H_

#include "basedefs.h"
#include <peff/containers/tree.h>
#include <memory>
#include <atomic>

namespace peff {
	struct SharedPtrControlBlock {
		std::atomic_size_t nWeakRefs = 0, nStrongRefs = 0;

		PEFF_FORCEINLINE SharedPtrControlBlock() {}
		inline virtual ~SharedPtrControlBlock() {}

		PEFF_FORCEINLINE void incStrongRef() noexcept {
			++nStrongRefs;
		}

		PEFF_FORCEINLINE void incWeakRef() noexcept {
			++nWeakRefs;
		}

		PEFF_FORCEINLINE void decStrongRef() noexcept {
			if (!--nStrongRefs) {
				onStrongRefZero();
				if (!nWeakRefs) {
					onRefZero();
				}
			}
		}

		PEFF_FORCEINLINE void decWeakRef() noexcept {
			if (!--nWeakRefs) {
				if (!nStrongRefs) {
					onRefZero();
				}
			}
		}

		virtual void onStrongRefZero() noexcept = 0;
		virtual void onRefZero() noexcept = 0;
	};

	template <typename T>
	class SharedPtr {
	public:
		struct DefaultSharedPtrControlBlock : public SharedPtrControlBlock {
			T *ptr;
			peff::RcObjectPtr<peff::Alloc> allocator;

			PEFF_FORCEINLINE DefaultSharedPtrControlBlock(peff::Alloc *allocator, T *ptr) noexcept : allocator(allocator), ptr(ptr) {}
			inline virtual ~DefaultSharedPtrControlBlock() {}

			inline virtual void onStrongRefZero() noexcept override {
				peff::destroyAndRelease<T>(allocator.get(), ptr, alignof(T));
			}

			inline virtual void onRefZero() noexcept override {
				peff::destroyAndRelease<DefaultSharedPtrControlBlock>(allocator.get(), this, alignof(DefaultSharedPtrControlBlock));
			}
		};

		class SharedFromThis {
		private:
			SharedPtrControlBlock *controlBlock;
			T *ptr;

			friend class SharedPtr<T>;

		public:
			PEFF_FORCEINLINE SharedPtr<T> sharedFromThis() noexcept {
				return SharedPtr<T>(controlBlock, ptr);
			}
		};

		SharedPtrControlBlock *controlBlock;
		T *ptr;

		PEFF_FORCEINLINE void reset() noexcept {
			if (controlBlock)
				controlBlock->decStrongRef();
			controlBlock = nullptr;
			ptr = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr() noexcept : controlBlock(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtrControlBlock *controlBlock, T *ptr) noexcept : controlBlock(controlBlock), ptr(ptr) {
			if (controlBlock) {
				controlBlock->incStrongRef();
			}

			if constexpr (std::is_base_of_v<SharedFromThis, T>) {
				((SharedFromThis*)ptr)->controlBlock = controlBlock;
				((SharedFromThis*)ptr)->ptr = ptr;
			}
		}
		PEFF_FORCEINLINE ~SharedPtr() {
			reset();
		}

		PEFF_FORCEINLINE SharedPtr(const SharedPtr<T> &rhs) noexcept : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			if (controlBlock) {
				controlBlock->incStrongRef();
			}
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtr<T> &&rhs) noexcept : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			rhs.controlBlock = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr<T> &operator=(const SharedPtr<T> &rhs) noexcept {
			reset();
			controlBlock = rhs.controlBlock;
			if (controlBlock) {
				controlBlock->incStrongRef();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE SharedPtr<T> &operator=(SharedPtr<T> &&rhs) noexcept {
			reset();
			controlBlock = rhs.controlBlock;
			ptr = rhs.ptr;
			rhs.controlBlock = nullptr;
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

		PEFF_FORCEINLINE operator bool() const noexcept {
			return (bool)ptr;
		}

		template <typename T1>
		PEFF_FORCEINLINE SharedPtr<T1> castTo() const noexcept {
			return SharedPtr<T1>(controlBlock, static_cast<T1 *>(ptr));
		}

		template <typename T1>
		PEFF_FORCEINLINE operator T1 *() const noexcept {
			return static_cast<T1 *>(ptr);
		}
	};

	template <typename T>
	class WeakPtr {
	public:
		SharedPtrControlBlock *controlBlock;
		T *ptr;

		PEFF_FORCEINLINE void reset() {
			if (controlBlock)
				controlBlock->decWeakRef();
			controlBlock = nullptr;
			ptr = nullptr;
		}

		PEFF_FORCEINLINE WeakPtr() noexcept : controlBlock(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE explicit WeakPtr(SharedPtrControlBlock *controlBlock, T *ptr) noexcept : controlBlock(controlBlock), ptr(ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE WeakPtr(const SharedPtr<T> &ptr) noexcept : controlBlock(ptr.controlBlock), ptr(ptr.ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE ~WeakPtr() {
			reset();
		}

		PEFF_FORCEINLINE WeakPtr(const WeakPtr<T> &rhs) noexcept : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE WeakPtr(WeakPtr<T> &&rhs) noexcept : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			rhs.controlBlock = nullptr;
			rhs.ptr = nullptr;
		}

		PEFF_FORCEINLINE WeakPtr<T> &operator=(const WeakPtr<T> &rhs) noexcept {
			reset();
			controlBlock = rhs.controlBlock;
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE WeakPtr<T> &operator=(WeakPtr<T> &&rhs) noexcept {
			reset();
			controlBlock = rhs.controlBlock;
			ptr = rhs.ptr;
			rhs.controlBlock = nullptr;
			rhs.ptr = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE SharedPtr<T> lock() const noexcept {
			if ((!controlBlock) || (!controlBlock->nStrongRefs))
				std::terminate();
			return SharedPtr<T>(controlBlock, ptr);
		}

		template <typename T1>
		PEFF_FORCEINLINE WeakPtr<T1> castTo() const noexcept {
			return WeakPtr<T1>(controlBlock, ptr + (((char *)static_cast<T *>(ptr)) - (char *)static_cast<T1 *>(ptr)));
		}

		PEFF_FORCEINLINE bool isValid() const noexcept {
			return controlBlock->nStrongRefs;
		}
	};

	template<typename T>
	using SharedFromThis = typename SharedPtr<T>::SharedFromThis;

	template <typename T, typename... Args>
	PEFF_FORCEINLINE SharedPtr<T> makeShared(peff::Alloc *allocator, Args &&...args) {
		T *ptr = peff::allocAndConstruct<T>(allocator, alignof(T), std::forward<Args>(args)...);
		if (!ptr)
			return {};
		peff::ScopeGuard releasePtrGuard([allocator, ptr]() noexcept {
			peff::destroyAndRelease<T>(allocator, ptr, alignof(T));
		});

		typename SharedPtr<T>::DefaultSharedPtrControlBlock *controlBlock =
			peff::allocAndConstruct<typename SharedPtr<T>::DefaultSharedPtrControlBlock>(
				allocator, alignof(typename SharedPtr<T>::DefaultSharedPtrControlBlock),
				allocator, ptr);
		if (!controlBlock)
			return {};
		releasePtrGuard.release();

		SharedPtr<T> p(controlBlock, ptr);

		return p;
	}
}

#endif
