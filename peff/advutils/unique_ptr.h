#ifndef _PEFF_ADVUTILS_UNIQUE_PTR_H_
#define _PEFF_ADVUTILS_UNIQUE_PTR_H_

#include "basedefs.h"
#include <peff/utils/pair.h>
#include <memory>
#include <atomic>

namespace peff {
	template <typename T, typename D = std::default_delete<T>>
	class UniquePtr {
	private:
		using ThisType = UniquePtr<T, D>;

		CompressedPair<T *, D> _pair;

	public:
		PEFF_FORCEINLINE D &deleter() noexcept {
			return _pair.second();
		}

		PEFF_FORCEINLINE const D &deleter() const noexcept {
			return _pair.second();
		}

		PEFF_FORCEINLINE void reset() noexcept {
			if (T *ptr = _pair.first(); ptr)
				_pair.second()(ptr);
		}

		PEFF_FORCEINLINE T *release() noexcept {
			T *p = _pair.first();
			_pair.first() = nullptr;
			return p;
		}

		PEFF_FORCEINLINE UniquePtr() : _pair(nullptr, D{}) {
		}
		PEFF_FORCEINLINE UniquePtr(T *ptr) noexcept : _pair(+ptr, D{}) {
		}
		UniquePtr(const ThisType &) = delete;
		PEFF_FORCEINLINE UniquePtr(ThisType &&other) noexcept : _pair(+other._pair.first(), std::move(other._pair.second())) {
			other._pair.first() = nullptr;
		}
		PEFF_FORCEINLINE ~UniquePtr() {
			reset();
		}

		PEFF_FORCEINLINE ThisType &operator=(T *_ptr) noexcept {
			reset();
			this->_pair.first() = _ptr;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) noexcept {
			reset();

			std::swap(_pair.first(), other._pair.first());
			_pair.second() = std::move(other._pair.second());

			return *this;
		}

		PEFF_FORCEINLINE T *get() const noexcept {
			return _pair.first();
		}
		PEFF_FORCEINLINE T *&getRef() noexcept {
			reset();
			return _pair.first();
		}
		PEFF_FORCEINLINE T *&getRefWithoutRelease() noexcept {
			return _pair.first();
		}
		PEFF_FORCEINLINE T *const &getRefWithoutRelease() const noexcept {
			return _pair.first();
		}
		PEFF_FORCEINLINE T **getAddressOf() noexcept {
			reset();
			return &_pair.first();
		}
		PEFF_FORCEINLINE T **getAddressOfWithoutRelease() noexcept {
			return &_pair.first();
		}
		PEFF_FORCEINLINE T *const *getAddressOfWithoutRelease() const noexcept {
			return &_pair.first();
		}
		PEFF_FORCEINLINE T *operator->() const noexcept {
			return _pair.first();
		}

		PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
			return _pair.first() < rhs._pair.first();
		}

		PEFF_FORCEINLINE explicit operator bool() const noexcept {
			return _pair.first();
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return _pair.first() == rhs._pair.first();
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return _pair.first() != rhs._pair.first();
		}
	};
}

#endif
