#ifndef _PEFF_BASE_RCOBJ_H_
#define _PEFF_BASE_RCOBJ_H_

#include "basedefs.h"
#include <mutex>
#include <cstddef>
#include <atomic>
#include <cassert>

namespace peff {
	struct BaseWeakRcObjectPtr;

	class RcObject {
	private:
		PEFF_BASE_API void _onRefZero() noexcept;

	public:
		std::atomic_size_t refCount = 0;

		BaseWeakRcObjectPtr *weakPtrs = nullptr;
		std::mutex weakPtrMutex;

		PEFF_BASE_API RcObject() noexcept;
		PEFF_BASE_API virtual ~RcObject();

		virtual void onRefZero() noexcept = 0;

		PEFF_FORCEINLINE size_t incRef() noexcept {
			return ++refCount;
		}

		PEFF_FORCEINLINE size_t decRef() noexcept {
			if (!(--refCount)) {
				_onRefZero();
				return 0;
			}
			return refCount;
		}
	};

	struct BaseWeakRcObjectPtr {
	protected:
		BaseWeakRcObjectPtr *_prev, *_next;
		RcObject *_ptr;
		PEFF_BASE_API void _reset();
		PEFF_BASE_API void _resetUnchecked();
		PEFF_BASE_API void _set(RcObject *ptr);

		friend class RcObject;

	public:
		PEFF_BASE_API BaseWeakRcObjectPtr(RcObject *ptr);
		PEFF_BASE_API ~BaseWeakRcObjectPtr();
	};

	template <typename T>
	struct WeakRcObjectPtr : public BaseWeakRcObjectPtr {
	public:
		PEFF_FORCEINLINE void reset() {
			_reset();
		}

		PEFF_FORCEINLINE WeakRcObjectPtr() : BaseWeakRcObjectPtr(nullptr) {}
		PEFF_FORCEINLINE WeakRcObjectPtr(RcObject *ptr) : BaseWeakRcObjectPtr(ptr) {}
		PEFF_FORCEINLINE WeakRcObjectPtr(WeakRcObjectPtr<T> &&ptr) : BaseWeakRcObjectPtr(ptr._ptr) {
			ptr->_ptr = nullptr;
		}
		PEFF_FORCEINLINE WeakRcObjectPtr(const WeakRcObjectPtr<T> &ptr) : BaseWeakRcObjectPtr(ptr._ptr) {
		}
		PEFF_FORCEINLINE ~WeakRcObjectPtr() {}

		PEFF_FORCEINLINE WeakRcObjectPtr<T> &operator=(RcObject *ptr) {
			if (ptr)
				_reset();
			_set(ptr);
			return *this;
		}
		PEFF_FORCEINLINE WeakRcObjectPtr<T> &operator=(WeakRcObjectPtr<T> &&ptr) {
			if (ptr)
				_reset();
			_set(ptr->_ptr);
			ptr->_ptr = nullptr;
			return *this;
		}
		PEFF_FORCEINLINE WeakRcObjectPtr<T> &operator=(const WeakRcObjectPtr<T> &ptr) {
			if (ptr)
				_reset();
			_set(ptr._ptr);
			return *this;
		}

		PEFF_FORCEINLINE T *operator->() const noexcept {
			assert(_ptr);
			return _ptr;
		}

		PEFF_FORCEINLINE T &operator*() const noexcept {
			assert(_ptr);
			return *_ptr;
		}

		PEFF_FORCEINLINE bool operator<(const WeakRcObjectPtr<T> &rhs) const noexcept {
			return _ptr < rhs._ptr;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _ptr;
		}
	};

	template <typename T>
	class RcObjectPtr {
	private:
		T *_ptr = nullptr;

		PEFF_FORCEINLINE void _setAndIncRef(T *_ptr) {
			this->_ptr = _ptr;
			if (_ptr)
				_ptr->incRef();
		}

	public:
		PEFF_FORCEINLINE void reset() noexcept {
			if (_ptr)
				_ptr->decRef();
			_ptr = nullptr;
		}

		PEFF_FORCEINLINE RcObjectPtr(T *ptr = nullptr) noexcept {
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

		PEFF_FORCEINLINE bool operator<(const RcObjectPtr<T> &rhs) const noexcept {
			return _ptr < rhs._ptr;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _ptr;
		}
	};

	struct RcObjectUniquePtrDeleter {
		PEFF_FORCEINLINE void operator()(RcObject *ptr) {
			if (ptr)
				ptr->onRefZero();
		}
	};
}

#endif
