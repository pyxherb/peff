#ifndef _PEFF_BASE_RCOBJ_H_
#define _PEFF_BASE_RCOBJ_H_

#include "basedefs.h"
#include <cstddef>
#include <atomic>

namespace peff {
	class RcObject {
	public:
		std::atomic_size_t refCount = 0;

		PEFF_BASE_API RcObject() noexcept;
		PEFF_BASE_API virtual ~RcObject();

		PEFF_BASE_API virtual void onRefZero() noexcept;

		PEFF_FORCEINLINE size_t incRef() noexcept {
			return ++refCount;
		}

		PEFF_FORCEINLINE size_t decRef() noexcept {
			if (!(--refCount))
				onRefZero();
			return refCount;
		}
	};

	template <typename T>
	class RcObjectPtr {
	private:
		T *_ptr = nullptr;

		PEFF_FORCEINLINE void _setAndIncRef(T *_ptr) {
			this->_ptr = _ptr;
			_ptr->incRef();
		}

	public:
		PEFF_FORCEINLINE void reset() noexcept {
			if (_ptr)
				_ptr->decRef();
			_ptr = nullptr;
		}

		PEFF_FORCEINLINE RcObjectPtr(T *ptr = nullptr) noexcept {
			if (ptr)
				_setAndIncRef(ptr);
		}
		PEFF_FORCEINLINE RcObjectPtr(const RcObjectPtr<T> &other) noexcept {
			_setAndIncRef(other._ptr);
		}
		PEFF_FORCEINLINE RcObjectPtr(RcObjectPtr<T> &&other) noexcept {
			_ptr = other._ptr;
			other._ptr = nullptr;
		}
		PEFF_FORCEINLINE ~RcObjectPtr() {
			reset();
		}

		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(T *_ptr) noexcept {
			reset();
			_setAndIncRef(_ptr);
			return *this;
		}
		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(const RcObjectPtr<T> &other) noexcept {
			reset();
			_setAndIncRef(other._ptr);
			return *this;
		}
		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(RcObjectPtr<T> &&other) noexcept {
			reset();
			_ptr = other._ptr;
			other._ptr = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE T *get() noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T *operator->() noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE const T *get() const noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE const T *operator->() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator<(const RcObjectPtr<T> &rhs) const noexcept {
			return _ptr < rhs._ptr;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _ptr;
		}
	};
}

#endif
