#ifndef _PEFF_BASE_RCOBJ_H_
#define _PEFF_BASE_RCOBJ_H_

#include "basedefs.h"
#include <mutex>
#include <cstddef>
#include <atomic>
#include <cassert>

namespace peff {
	class RcObject {
	protected:
		PEFF_BASE_API virtual void onIncRef(size_t counter);
		PEFF_BASE_API virtual void onDecRef(size_t counter);

	public:
		std::atomic_size_t refCount;

		PEFF_BASE_API RcObject() noexcept;
		PEFF_BASE_API virtual ~RcObject();

		virtual void onRefZero() noexcept = 0;

		PEFF_FORCEINLINE size_t incRef(size_t counter) noexcept {
			onIncRef(counter);
			return ++refCount;
		}

		PEFF_FORCEINLINE size_t decRef(size_t counter) noexcept {
			if (counter != SIZE_MAX)
				onDecRef(counter);
			if (!(--this->refCount)) {
				onRefZero();
				return 0;
			}
			return this->refCount;
		}
	};

	PEFF_BASE_API extern std::atomic_size_t g_rcObjectPtrCounter;

	template <typename T>
	class RcObjectPtr {
	public:
		size_t _counter = SIZE_MAX;

	private:
		using ThisType = RcObjectPtr<T>;

		T *_ptr = nullptr;

		PEFF_FORCEINLINE void _setAndIncRef(T *_ptr) {
			_counter = g_rcObjectPtrCounter++;
			_ptr->incRef(_counter);
			this->_ptr = _ptr;
		}

	public:
		PEFF_FORCEINLINE void reset() noexcept {
			if (_ptr) {
				_ptr->decRef(_counter);
			}
			_ptr = nullptr;
		}

		PEFF_FORCEINLINE RcObjectPtr() : _ptr(nullptr) {
		}
		PEFF_FORCEINLINE RcObjectPtr(T *ptr) noexcept : _ptr(nullptr) {
			if (ptr) {
				_setAndIncRef(ptr);
			}
		}
		PEFF_FORCEINLINE RcObjectPtr(const ThisType &other) noexcept : _ptr(nullptr) {
			if (other._ptr) {
				_setAndIncRef(other._ptr);
			}
		}
		PEFF_FORCEINLINE RcObjectPtr(ThisType &&other) noexcept {
			_ptr = other._ptr;
			_counter = other._counter;
			other._ptr = nullptr;
			other._counter = SIZE_MAX;
		}
		PEFF_FORCEINLINE ~RcObjectPtr() {
			reset();
		}

		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(T *_ptr) noexcept {
			reset();
			if (_ptr) {
				_setAndIncRef(_ptr);
			}
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
			_counter = other._counter;

			other._ptr = nullptr;
			other._counter = SIZE_MAX;

			return *this;
		}

		PEFF_FORCEINLINE T *get() const noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T *&getRef() noexcept {
			reset();
			return _ptr;
		}
		PEFF_FORCEINLINE T *&getRefWithoutRelease() noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T *const &getRefWithoutRelease() const noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T **getAddressOf() noexcept {
			reset();
			return &_ptr;
		}
		PEFF_FORCEINLINE T **getAddressOfWithoutRelease() noexcept {
			return &_ptr;
		}
		PEFF_FORCEINLINE T *const *getAddressOfWithoutRelease() const noexcept {
			return &_ptr;
		}
		PEFF_FORCEINLINE T *operator->() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
			return _ptr < rhs._ptr;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return _ptr == rhs._ptr;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return _ptr != rhs._ptr;
		}
	};

	struct RcObjectUniquePtrDeleter {
		PEFF_FORCEINLINE void operator()(RcObject *ptr) const {
			if (ptr)
				ptr->onRefZero();
		}
	};
}

#endif
