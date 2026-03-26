#ifndef _PEFF_UTILS_BINARY_HEAP_H_
#define _PEFF_UTILS_BINARY_HEAP_H_

#include "dynarray.h"

namespace peff {
	template <typename T, typename Ord = std::less<T>>
	class BinaryHeapArray {
	private:
		using HeapArray = peff::DynArray<T>;
		HeapArray _heap_array;
		Ord _ord;

		PEFF_FORCEINLINE void _sort() noexcept {
			size_t i = _heap_array.size();
			if (!i)
				return;
			i -= 1;
			while (i > 1) {
				T &l = _heap_array.at(i), &r = _heap_array.at(i >> 1);
				if (!_ord(l, r))
					break;
				std::swap(l, r);
				i >>= 1;
			}
		}

	public:
		using Iterator = HeapArray::Iterator;
		using ConstIterator = HeapArray::ConstIterator;

		PEFF_FORCEINLINE BinaryHeapArray(peff::Alloc *allocator, Ord &&ord = {}) : _heap_array(allocator), _ord(ord) {
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&data) noexcept {
			if (!_heap_array.push_back(std::move(data)))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE void pop_front() noexcept {
			_heap_array.pop_front();
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pop_front_and_shrink() noexcept {
			if (!_heap_array.pop_front_and_shrink())
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE const T &front() const noexcept {
			return _heap_array.at(1);
		}

		PEFF_FORCEINLINE void pop_back() noexcept {
			_heap_array.pop_back();
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pop_back_and_shrink() noexcept {
			if (!_heap_array.pop_back_and_shrink())
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE bool shrink_to_fit() noexcept {
			return _heap_array.shrink_to_fit();
		}

		PEFF_FORCEINLINE const T &back() const noexcept {
			return _heap_array.at(1);
		}

		PEFF_FORCEINLINE const T &at(size_t index) const noexcept {
			return _heap_array.at(index);
		}

		PEFF_FORCEINLINE void remove(size_t index) noexcept {
			_heap_array.erase_range(index, index + 1);
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool remove_and_shrink(size_t index) noexcept {
			if (!_heap_array.erase_range_and_shrink(index, index + 1))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE void erase_range(size_t begin, size_t end) noexcept {
			_heap_array.erase_range(begin, end);
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool erase_range_and_shrink(size_t begin, size_t end) noexcept {
			if (!_heap_array.erase_range_and_shrink(begin, end))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _heap_array.size();
		}

		PEFF_FORCEINLINE Iterator begin() noexcept {
			return _heap_array.begin();
		}

		PEFF_FORCEINLINE Iterator end() noexcept {
			return _heap_array.end();
		}

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			return _heap_array.begin_const();
		}

		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return _heap_array.end_const();
		}

		PEFF_FORCEINLINE ConstIterator begin_const() const noexcept {
			return _heap_array.begin_const();
		}

		PEFF_FORCEINLINE ConstIterator end_const() const noexcept {
			return _heap_array.end_const();
		}
	};
}

#endif
