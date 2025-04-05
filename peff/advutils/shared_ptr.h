#ifndef _PEFF_ADVUTILS_SHARED_PTR_H_
#define _PEFF_ADVUTILS_SHARED_PTR_H_

#include "basedefs.h"
#include <peff/containers/tree.h>
#include <memory>
#include <atomic>

namespace peff {
	struct SharedPtrControlBlock {
		std::atomic_size_t nWeakRefs, nStrongRefs;

		PEFF_FORCEINLINE SharedPtrControlBlock() : nWeakRefs(0), nStrongRefs(0) {}
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

			PEFF_FORCEINLINE DefaultSharedPtrControlBlock(peff::Alloc *allocator, T *ptr) : allocator(allocator), ptr(ptr) {}
			inline virtual ~DefaultSharedPtrControlBlock() {}

			inline virtual void onStrongRefZero() noexcept override {
				peff::destroyAndRelease<T>(allocator.get(), ptr, sizeof(std::max_align_t));
			}

			inline virtual void onRefZero() noexcept override {
				peff::destroyAndRelease<DefaultSharedPtrControlBlock>(allocator.get(), this, sizeof(std::max_align_t));
			}
		};

		class SharedFromThis {
		private:
			SharedPtrControlBlock *controlBlock;
			T *ptr;

			friend class SharedPtr<T>;

		public:
			PEFF_FORCEINLINE SharedPtr<T> sharedFromThis() {
				return SharedPtr<T>(controlBlock, ptr);
			}
		};

		SharedPtrControlBlock *controlBlock;
		T *ptr;

		PEFF_FORCEINLINE void reset() {
			if (controlBlock)
				controlBlock->decStrongRef();
			controlBlock = nullptr;
			ptr = 0;
		}

		PEFF_FORCEINLINE SharedPtr() : controlBlock(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtrControlBlock *controlBlock, T *ptr) : controlBlock(controlBlock), ptr(ptr) {
			if constexpr (std::is_base_of_v<SharedFromThis, T>) {
				((SharedFromThis*)ptr)->controlBlock = controlBlock;
				((SharedFromThis*)ptr)->ptr = ptr;
			}

			if (controlBlock) {
				controlBlock->incStrongRef();
			}
		}
		PEFF_FORCEINLINE ~SharedPtr() {
			reset();
		}

		PEFF_FORCEINLINE SharedPtr(const SharedPtr<T> &rhs) : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			if (controlBlock) {
				controlBlock->incStrongRef();
			}
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtr<T> &&rhs) : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			rhs.controlBlock = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr<T> &operator=(const SharedPtr<T> &rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			if (controlBlock) {
				controlBlock->incStrongRef();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE SharedPtr<T> &operator=(SharedPtr<T> &&rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			ptr = rhs.ptr;
			rhs.controlBlock = nullptr;
			rhs.ptr = 0;

			return *this;
		}

		PEFF_FORCEINLINE T *get() const {
			return (T *)ptr;
		}

		PEFF_FORCEINLINE T *operator->() const {
			return get();
		}
		PEFF_FORCEINLINE T &operator*() const {
			return *get();
		}

		PEFF_FORCEINLINE operator bool() const {
			return (bool)ptr;
		}

		template <typename T1>
		PEFF_FORCEINLINE SharedPtr<T1> castTo() const {
			return SharedPtr<T1>(controlBlock, static_cast<T1 *>(ptr));
		}

		template <typename T1>
		PEFF_FORCEINLINE operator T1 *() const {
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
			ptr = 0;
		}

		PEFF_FORCEINLINE WeakPtr() : controlBlock(nullptr), ptr(nullptr) {
		}
		PEFF_FORCEINLINE WeakPtr(SharedPtrControlBlock *controlBlock, T *ptr) : controlBlock(controlBlock), ptr(ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE WeakPtr(const SharedPtr<T> &ptr) : controlBlock(ptr.controlBlock), ptr(ptr.ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE ~WeakPtr() {
			reset();
		}

		PEFF_FORCEINLINE WeakPtr(const WeakPtr<T> &rhs) : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
		}
		PEFF_FORCEINLINE WeakPtr(WeakPtr<T> &&rhs) : controlBlock(rhs.controlBlock), ptr(rhs.ptr) {
			rhs.controlBlock = nullptr;
		}

		PEFF_FORCEINLINE WeakPtr<T> &operator=(const WeakPtr<T> &rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			if (controlBlock) {
				controlBlock->incWeakRef();
			}
			ptr = rhs.ptr;

			return *this;
		}
		PEFF_FORCEINLINE WeakPtr<T> &operator=(WeakPtr<T> &&rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			ptr = rhs.ptr;
			rhs.controlBlock = nullptr;
			rhs.ptr = 0;

			return *this;
		}

		PEFF_FORCEINLINE SharedPtr<T> lock() const {
			if ((!controlBlock) || (!controlBlock->nStrongRefs))
				std::terminate();
			return SharedPtr<T>(controlBlock, ptr);
		}

		template <typename T1>
		PEFF_FORCEINLINE WeakPtr<T1> castTo() const {
			return WeakPtr<T1>(controlBlock, ptr + (((char *)static_cast<T *>(ptr)) - (char *)static_cast<T1 *>(ptr)));
		}
	};

	template<typename T>
	using SharedFromThis = typename SharedPtr<T>::SharedFromThis;

	template <typename T, typename... Args>
	PEFF_FORCEINLINE SharedPtr<T> makeShared(peff::Alloc *allocator, Args &&...args) {
		T *ptr = peff::allocAndConstruct<T>(allocator, sizeof(std::max_align_t), std::forward<Args>(args)...);
		if (!ptr)
			return {};
		peff::ScopeGuard releasePtrGuard([allocator, ptr]() noexcept {
			peff::destroyAndRelease<T>(allocator, ptr, sizeof(std::max_align_t));
		});

		typename SharedPtr<T>::DefaultSharedPtrControlBlock *controlBlock =
			peff::allocAndConstruct<typename SharedPtr<T>::DefaultSharedPtrControlBlock>(
				allocator, sizeof(std::max_align_t),
				allocator, ptr);
		if (!controlBlock)
			return {};
		releasePtrGuard.release();

		SharedPtr<T> p(controlBlock, ptr);

		return p;
	}
}

#endif
