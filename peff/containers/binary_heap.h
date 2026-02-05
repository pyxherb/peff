#ifndef _PEFF_UTILS_BINARY_HEAP_H_
#define _PEFF_UTILS_BINARY_HEAP_H_

#include "dynarray.h"

namespace peff {
	template <typename T, typename Ord = std::less<T>>
	class BinaryHeapArray {
	private:
		using HeapArray = peff::DynArray<T>;
		HeapArray _heapArray;
		Ord _ord;

		PEFF_FORCEINLINE void _sort() noexcept {
			size_t i = _heapArray.size();
			if (!i)
				return;
			i -= 1;
			while (i > 1) {
				T &l = _heapArray.at(i), &r = _heapArray.at(i >> 1);
				if (!_ord(l, r))
					break;
				std::swap(l, r);
				i >>= 1;
			}
		}

	public:
		using Iterator = HeapArray::Iterator;
		using ConstIterator = HeapArray::ConstIterator;

		PEFF_FORCEINLINE BinaryHeapArray(peff::Alloc *allocator, Ord &&ord = {}) : _heapArray(allocator), _ord(ord) {
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&data) noexcept {
			if (!_heapArray.pushBack(std::move(data)))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE void popFront() noexcept {
			_heapArray.popFront();
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popFrontAndShrink() noexcept {
			if (!_heapArray.popFrontAndShrink())
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE const T &front() const noexcept {
			return _heapArray.at(1);
		}
		
		PEFF_FORCEINLINE void popBack() noexcept {
			_heapArray.popBack();
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popBackAndShrink() noexcept {
			if (!_heapArray.popBackAndShrink())
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE bool shrinkToFit() noexcept {
			return _heapArray.shrinkToFit();
		}

		PEFF_FORCEINLINE const T &back() const noexcept {
			return _heapArray.at(1);
		}

		PEFF_FORCEINLINE const T &at(size_t index) const noexcept {
			return _heapArray.at(index);
		}

		PEFF_FORCEINLINE void remove(size_t index) noexcept {
			_heapArray.eraseRange(index, index + 1);
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool removeAndShrink(size_t index) noexcept {
			if (!_heapArray.eraseRangeAndShrink(index, index + 1))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE void eraseRange(size_t begin, size_t end) noexcept {
			_heapArray.eraseRange(begin, end);
			_sort();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool eraseRangeAndShrink(size_t begin, size_t end) noexcept {
			if (!_heapArray.eraseRangeAndShrink(begin, end))
				return false;
			_sort();
			return true;
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _heapArray.size();
		}

		PEFF_FORCEINLINE Iterator begin() noexcept {
			return _heapArray.begin();
		}

		PEFF_FORCEINLINE Iterator end() noexcept {
			return _heapArray.end();
		}

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			return _heapArray.beginConst();
		}

		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return _heapArray.endConst();
		}

		PEFF_FORCEINLINE ConstIterator beginConst() const noexcept {
			return _heapArray.beginConst();
		}

		PEFF_FORCEINLINE ConstIterator endConst() const noexcept {
			return _heapArray.endConst();
		}
	};
}

#endif
