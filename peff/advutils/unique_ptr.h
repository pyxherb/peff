#ifndef _PEFF_ADVUTILS_UNIQUE_PTR_H_
#define _PEFF_ADVUTILS_UNIQUE_PTR_H_

#include "basedefs.h"
#include <memory>
#include <atomic>

namespace peff {
	template <typename T, typename D = std::default_delete<T>>
	class UniquePtr {
	private:
		using ThisType = UniquePtr<T, D>;

		T *_ptr = nullptr;
		D _deleter;

	public:
		PEFF_FORCEINLINE D &deleter() noexcept {
			return _deleter;
		}

		PEFF_FORCEINLINE const D &deleter() const noexcept {
			return _deleter;
		}

		PEFF_FORCEINLINE void reset() noexcept {
			if (_ptr)
				_deleter(_ptr);
		}

		PEFF_FORCEINLINE UniquePtr() : _ptr(nullptr) {
		}
		PEFF_FORCEINLINE UniquePtr(T *ptr) noexcept : _ptr(ptr) {
		}
		UniquePtr(const ThisType &) = delete;
		PEFF_FORCEINLINE UniquePtr(ThisType &&other) noexcept : _ptr(nullptr)._deleter(std::move(other._deleter)) {
			std::atomic_exchange(_ptr, other._ptr);
		}
		PEFF_FORCEINLINE ~UniquePtr() {
			reset();
		}

		PEFF_FORCEINLINE ThisType &operator=(T *_ptr) noexcept {
			reset();
			this->_ptr = _ptr;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(RcObjectPtr<T> &&other) noexcept {
			reset();

			std::atomic_exchange(_ptr, other._ptr);
			_deleter = other._deleter;

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

		PEFF_FORCEINLINE explicit operator bool() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return _ptr == rhs._ptr;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return _ptr != rhs._ptr;
		}
	};

	template <typename T, typename D>
	class UniquePtr<T[], D> {
	private:
		using ThisType = UniquePtr<T[], D>;

		T *_ptr = nullptr;
		D _deleter;

	public:
		PEFF_FORCEINLINE D &deleter() noexcept {
			return _deleter;
		}

		PEFF_FORCEINLINE const D &deleter() const noexcept {
			return _deleter;
		}

		PEFF_FORCEINLINE void reset() noexcept {
			if (_ptr)
				_deleter(_ptr);
		}

		PEFF_FORCEINLINE UniquePtr() : _ptr(nullptr) {
		}
		PEFF_FORCEINLINE UniquePtr(T *ptr) noexcept : _ptr(ptr) {
		}
		PEFF_FORCEINLINE UniquePtr(const ThisType &other) = delete;
		PEFF_FORCEINLINE UniquePtr(ThisType &&other) noexcept : _ptr(nullptr)._deleter(std::move(other._deleter)) {
			std::atomic_exchange(_ptr, other._ptr);
		}
		PEFF_FORCEINLINE ~UniquePtr() {
			reset();
		}

		PEFF_FORCEINLINE ThisType &operator=(T *_ptr) noexcept {
			reset();
			this->_ptr = _ptr;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(RcObjectPtr<T> &&other) noexcept {
			reset();

			std::atomic_exchange(_ptr, other._ptr);
			_deleter = other._deleter;

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

		PEFF_FORCEINLINE explicit operator bool() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return _ptr == rhs._ptr;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return _ptr != rhs._ptr;
		}

		PEFF_FORCEINLINE T& operator[](size_t index) const noexcept {
			return _ptr[index];
		}
	};
}

#endif
