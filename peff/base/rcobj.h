#ifndef _PEFF_BASE_RCOBJ_H_
#define _PEFF_BASE_RCOBJ_H_

#include "basedefs.h"
#include <mutex>
#include <cstddef>
#include <atomic>
#include <cassert>
#include <type_traits>

namespace peff {
	template <typename T, typename V = void, typename W = void>
	struct IsRcObject : std::false_type {
	};

	template <typename T>
	struct IsRcObject<T, std::void_t<decltype(std::declval<T>().incRef((size_t)0))>, std::void_t<decltype(std::declval<T>().decRef((size_t)0))>> : std::true_type {
	};

	PEFF_BASE_API extern std::atomic_size_t g_rcObjectPtrCounter;

#if __cplusplus >= 202002L
	template <typename T>
	concept RcObjectConcept = requires(T * rcObject) {
		rcObject->incRef(0);
		rcObject->decRef(0);
	};
#endif

	template <typename T>
	class RcObjectPtr {
	public:
		size_t _counter = SIZE_MAX;

	private:
		using ThisType = RcObjectPtr<T>;

		T *_ptr = nullptr;

		PEFF_FORCEINLINE void _setAndIncRef(T *_ptr)
			PEFF_REQUIRES_CONCEPT(RcObjectConcept<T>) {
			_counter = g_rcObjectPtrCounter++;
			_ptr->incRef(_counter);
			this->_ptr = _ptr;
		}

	public:
		PEFF_FORCEINLINE void reset() noexcept
			PEFF_REQUIRES_CONCEPT(RcObjectConcept<T>) {
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
}

#endif
