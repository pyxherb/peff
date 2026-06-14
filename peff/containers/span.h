#ifndef _PEFF_CONTAINERS_SPAN_H_
#define _PEFF_CONTAINERS_SPAN_H_

#include "basedefs.h"
#include <peff/base/assert.h>
#include <type_traits>

namespace peff {
	template <typename T>
	class Span {
	private:
		T *_ptr;
		size_t _length;

	public:
		Span() : _ptr(nullptr), _length(0) {}
		Span(T *ptr, size_t length) : _ptr(ptr), _length(length) {}
		template <size_t N>
		Span(T array[N]) : _ptr(array), _length(N) {}
		~Span() = default;
		Span(const Span<T> &) = default;
		Span(Span<T> &&) = default;

		PEFF_FORCEINLINE T *data() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE T &at(size_t index) const noexcept {
			return _ptr + index;
		}

		PEFF_FORCEINLINE T &operator[](size_t index) const noexcept {
			return _ptr + index;
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _length;
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return !_length;
		}

		PEFF_FORCEINLINE T &front() const noexcept {
			PEFF_ASSERT(_length, "The span is empty");
			return _ptr;
		}

		PEFF_FORCEINLINE T &back() const noexcept {
			PEFF_ASSERT(_length, "The span is empty");
			return _ptr + _length - 1;
		}

		PEFF_FORCEINLINE Span<T> subspan(size_t index, size_t length) const noexcept {
			PEFF_ASSERT(index + length <= _length, "Out of span range");
			return { _ptr + index, length };
		}
	};
}

#endif
