#ifndef _PEFF_UTILS_MEMORY_H_
#define _PEFF_UTILS_MEMORY_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <memory>
#include <atomic>

namespace peff {
	template <typename T>
	class SharedPtr {
	public:
		struct SharedPtrControlBlock {
			T *ptr;
			std::atomic_size_t nWeakRefs, nStrongRefs;

			PEFF_FORCEINLINE SharedPtrControlBlock(T *ptr) : ptr(ptr), nWeakRefs(0), nStrongRefs(0) {}
			inline virtual ~SharedPtrControlBlock() {}

			PEFF_FORCEINLINE void incStrongRef() noexcept {
				++nStrongRefs;
			}

			PEFF_FORCEINLINE void incWeakRef() noexcept {
				++nWeakRefs;
			}

			PEFF_FORCEINLINE void decStrongRef() noexcept {
				if (!--nStrongRefs) {
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

			inline virtual void onRefZero() noexcept {
				delete (T *)ptr;
				delete this;
			}
		};

		struct DefaultSharedPtrControlBlock : public SharedPtrControlBlock {
			peff::RcObjectPtr<peff::Alloc> allocator;

			PEFF_FORCEINLINE DefaultSharedPtrControlBlock(peff::Alloc *allocator, T *ptr) : SharedPtrControlBlock(ptr), allocator(allocator) {}
			inline virtual ~DefaultSharedPtrControlBlock() {}

			inline virtual void onRefZero() noexcept override {
				peff::destroyAndRelease<T>(allocator.get(), this->ptr, sizeof(std::max_align_t));
			}
		};

		SharedPtrControlBlock *controlBlock;

		PEFF_FORCEINLINE void reset() {
			if (controlBlock)
				controlBlock->decStrongRef();
			controlBlock = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr() : controlBlock(nullptr) {
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtrControlBlock *controlBlock) : controlBlock(controlBlock) {
			controlBlock->incStrongRef();
		}
		PEFF_FORCEINLINE ~SharedPtr() {
			reset();
		}

		PEFF_FORCEINLINE SharedPtr(const SharedPtr<T> &rhs) : controlBlock(rhs.controlBlock) {
			controlBlock->incStrongRef();
		}
		PEFF_FORCEINLINE SharedPtr(SharedPtr<T> &&rhs) : controlBlock(rhs.controlBlock) {
			rhs.controlBlock = nullptr;
		}

		PEFF_FORCEINLINE SharedPtr<T> &operator=(const SharedPtr<T> &rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			if (controlBlock) {
				controlBlock->incStrongRef();
			}

			return *this;
		}
		PEFF_FORCEINLINE SharedPtr<T> &operator=(SharedPtr<T> &&rhs) {
			reset();
			controlBlock = rhs.controlBlock;
			rhs.controlBlock = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE SharedPtr<T> &operator=(SharedPtrControlBlock *controlBlock) {
			reset();
			this->controlBlock = controlBlock;
			if (controlBlock) {
				controlBlock->incStrongRef();
			}

			return *this;
		}

		PEFF_FORCEINLINE T *operator->() const {
			return controlBlock->ptr;
		}
		PEFF_FORCEINLINE T &operator*() const {
			return *controlBlock->ptr;
		}
	};

	template <typename T, typename... Args>
	SharedPtr<T> makeShared(peff::Alloc *allocator, Args &&...args) {
		T *ptr = peff::allocAndConstruct<T>(allocator, sizeof(std::max_align_t), std::forward<Args>(args)...);
		if (!ptr)
			return nullptr;
		peff::ScopeGuard releasePtrGuard([allocator, ptr]() noexcept {
			peff::destroyAndRelease<T>(allocator, ptr, sizeof(std::max_align_t));
		});

		typename SharedPtr<T>::DefaultSharedPtrControlBlock *controlBlock =
			peff::allocAndConstruct<typename SharedPtr<T>::DefaultSharedPtrControlBlock>(
				allocator, sizeof(std::max_align_t),
				allocator, ptr);
		if (!controlBlock)
			return nullptr;
		releasePtrGuard.release();

		return SharedPtr<T>(controlBlock);
	}
}

#endif
