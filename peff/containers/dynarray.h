#ifndef _PEFF_UTILS_DYNARRAY_H_
#define _PEFF_UTILS_DYNARRAY_H_

#include <cassert>
#include <cstring>
#include <peff/base/alloc.h>
#include <peff/base/misc.h>

namespace peff {
	/// @brief The dynamic array type.
	/// @tparam T Type of the elements.
	template <typename T>
	class DynArray {
	public:
		using Iterator = T *;
		using ConstIterator = const T *;

		size_t _length = 0;
		size_t _capacity = 0;
		peff::RcObjectPtr<Alloc> _allocator;
		T *_data;

		PEFF_FORCEINLINE static int _checkCapacity(size_t length, size_t capacity) {
			if (length > capacity)
				return 1;
			if (capacity < (length >> 1))
				return -1;
			return 0;
		}

		PEFF_FORCEINLINE void _moveData(T *newData, T *oldData, size_t length) noexcept {
			if constexpr (std::is_trivially_move_assignable_v<T>) {
				memmove(newData, oldData, sizeof(T) * length);
			} else {
				if (newData + length < oldData) {
					for (size_t i = 0; i < length; ++i) {
						newData[i] = std::move(oldData[i]);
					}
				} else {
					for (size_t i = length; i > 0; --i) {
						newData[i - 1] = std::move(oldData[i - 1]);
					}
				}
			}
		}

		PEFF_FORCEINLINE void _moveDataUninitialized(T *newData, T *oldData, size_t length) noexcept {
			if constexpr (std::is_trivially_move_constructible_v<T>) {
				memmove(newData, oldData, sizeof(T) * length);
			} else {
				if (newData + length < oldData) {
					for (size_t i = 0; i < length; ++i) {
						constructAt<T>(&newData[i], std::move(oldData[i]));
					}
				} else {
					for (size_t i = length; i > 0; --i) {
						constructAt<T>(&newData[i - 1], std::move(oldData[i - 1]));
					}
				}
			}
		}

		PEFF_FORCEINLINE void _constructData(T *newData, size_t length) {
			if constexpr (!std::is_trivially_constructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					constructAt<T>(&newData[i]);
				}
			}
		}

		PEFF_FORCEINLINE void _destructData(T *originalData, size_t length) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					std::destroy_at<T>(&originalData[i]);
				}
			}
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _expandTo(
			T *newData,
			size_t length) {
			assert(length > _length);

			if constexpr (construct) {
				// Because construction of new objects may throw exceptions,
				// we choose to construct the new objects first.
				size_t idxLastConstructedObject;
				ScopeGuard scopeGuard(
					[this, &idxLastConstructedObject, newData]() noexcept {
						for (size_t i = _length;
							i < idxLastConstructedObject;
							++i) {
							std::destroy_at<T>(&newData[i]);
						}
					});

				for (size_t i = _length;
					i < length;
					++i) {
					idxLastConstructedObject = i;
					constructAt<T>(&newData[i]);
				}

				scopeGuard.release();
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, _length);
			}

			return true;
		}

		PEFF_FORCEINLINE void _shrink(
			T *newData,
			size_t length) noexcept {
			assert(length < _length);

			for (size_t i = length; i < _length; ++i) {
				std::destroy_at<T>(&_data[i]);
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, length);
			}
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _resize(size_t length, bool forceResizeCapacity) {
			if (length == _length)
				return true;

			if (!length) {
				_clear();
				return true;
			}

			int capacityStatus = _checkCapacity(length, _capacity);
			if (capacityStatus > 0) {
				size_t newCapacity = _capacity ? (_capacity << 1) : length;

				while (newCapacity < length)
					newCapacity <<= 1;

				size_t newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData;
				bool clearOldData = true;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					if (_data) {
						if (!(newData = (T *)_allocator->realloc(_data, sizeof(T) * _capacity, alignof(T), newCapacityTotalSize, alignof(T))))
							return false;
						clearOldData = false;
					} else {
						if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
							return false;
					}
				} else {
					if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
						return false;

					ScopeGuard scopeGuard(
						[this, newCapacityTotalSize, newData]() noexcept {
							_allocator->release(newData, newCapacityTotalSize, alignof(T));
						});

					if (!_expandTo<construct>(newData, length))
						return false;

					scopeGuard.release();
				}

				if (clearOldData)
					_clear();
				_capacity = newCapacity;
				_data = newData;
			} else if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1;

				while ((newCapacity >> 1) > length)
					newCapacity >>= 1;

				size_t newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData;
				bool clearOldData = true;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					if (_data) {
						if (!(newData = (T *)_allocator->realloc(_data, sizeof(T) * _capacity, alignof(T), newCapacityTotalSize, alignof(T))))
							return false;
						clearOldData = false;
					} else {
						if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
							return false;
					}
				} else {
					if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
						return false;
				}

				if (!newData) {
					if (forceResizeCapacity) {
						return false;
					}
					_length = length;
					return true;
				}

				if constexpr (std::is_trivially_move_assignable_v<T>) {
				} else {
					ScopeGuard scopeGuard(
						[this, newCapacityTotalSize, newData]() noexcept {
							_allocator->release(newData, newCapacityTotalSize, alignof(T));
						});

					_shrink(newData, length);

					scopeGuard.release();
				}

				if (clearOldData)
					_clear();
				_capacity = newCapacity;
				_data = newData;
			} else {
				if (length > _length) {
					if constexpr (!std::is_trivially_constructible_v<T>) {
						if (!_expandTo<construct>(_data, length))
							return false;
					}
				} else {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						_shrink(_data, length);
					}
				}
			}

			_length = length;
			return true;
		}

		PEFF_FORCEINLINE void _clear() {
			if constexpr (!std::is_trivial_v<T>) {
				for (size_t i = 0; i < _length; ++i)
					std::destroy_at<T>(&_data[i]);
			}
			if (_capacity) {
				_allocator->release(_data, sizeof(T) * _capacity, alignof(T));
			}

			_data = nullptr;
			_length = 0;
			_capacity = 0;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool eraseRange(size_t idxStart, size_t idxEnd) {
			assert(idxStart < _length);
			assert(idxEnd <= _length);

			const size_t gapLength = idxEnd - idxStart;
			const size_t postGapLength = _length - idxEnd;
			const size_t newLength = _length - gapLength;

			int capacityStatus = _checkCapacity(newLength, _capacity);
			assert(capacityStatus <= 0);
			if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * idxStart);
					memmove(newData + idxStart, _data + idxEnd, sizeof(T) * postGapLength);
				} else {
					ScopeGuard scopeGuard(
						[this, newCapacityTotalSize, newData]() noexcept {
							_allocator->release(newData, newCapacityTotalSize, alignof(T));
						});

					_moveData(newData, _data, idxStart);
					_moveData(newData + idxStart, _data + idxEnd, postGapLength);

					scopeGuard.release();
				}

				if (_data)
					_allocator->release(_data, sizeof(T) * _capacity, alignof(T));
				_capacity = newCapacity;
				_data = newData;
			} else {
				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(&_data[idxStart], &_data[idxEnd], postGapLength * sizeof(T));
				} else {
					for (size_t i = idxStart; i < idxEnd; ++i)
						std::destroy_at<T>(&_data[i]);
					if (idxEnd < _length)
						_moveDataUninitialized(&_data[idxStart], &_data[idxEnd], postGapLength);
				}
			}

			_length = newLength;

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool extractRange(size_t idxStart, size_t idxEnd) {
			const size_t newLength = idxEnd - idxStart;

			if (newLength == _length)
				return true;

			if (!newLength) {
				clear();
				return true;
			}

			if (idxStart) {
				_moveData(_data, _data + idxStart, newLength);
				if (!_resize<false>(newLength, true)) {
					// Change the length, but keep the capacity unchanged.
					_length = newLength;
				}
			} else {
				if (!_resize<false>(newLength, true)) {
					// Destruct the trailing elements.
					_destructData(_data + idxStart, idxEnd - idxStart);
				}
			}

			return _resize<false>(newLength, true);
		}

		PEFF_FORCEINLINE void extractRangeWithoutShrink(size_t idxStart, size_t idxEnd) {
			const size_t newLength = idxEnd - idxStart;

			if (newLength == _length)
				return;

			if (!newLength) {
				clear();
				return;
			}

			if (idxStart) {
				_moveData(_data, _data + idxStart, newLength);
				if (!_resize<false>(newLength, false)) {
					// Change the length, but keep the capacity unchanged.
					_length = newLength;
				}
			} else {
				if (!_resize<false>(newLength, false)) {
					// Destruct the trailing elements.
					_destructData(_data + idxStart, idxEnd - idxStart);
				}
			}
		}
		/// @brief Reserve an area in front of specified index.
		/// @param index Index to be reserved.
		/// @param length Length of space to reserve.
		/// @param construct Determines if to construct objects.
		/// @return Pointer to the reserved area.
		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE T *_insertRange(
			size_t index,
			size_t length) {
			const size_t
				oldLength = _length,
				newLength = _length + length;

			if (!_resize<construct>(newLength, false))
				return nullptr;

			T *gapStart = &_data[index];

			if (std::is_trivially_move_assignable_v<T>) {
				if (index < oldLength) {
					memmove(&_data[index + length], gapStart, sizeof(T) * (oldLength - index));
				}
			} else {
				if (index < oldLength) {
					_moveDataUninitialized(
						&_data[index + length],
						gapStart,
						oldLength - index);
				}

				if constexpr (construct) {
					_constructData(gapStart, length);
				}
			}

			return gapStart;
		}

		using ThisType = DynArray<T>;

	public:
		PEFF_FORCEINLINE DynArray(Alloc *allocator) : _allocator(allocator), _data(nullptr) {
		}
		PEFF_FORCEINLINE DynArray(ThisType &&rhs) noexcept : _allocator(std::move(rhs._allocator)), _data(std::move(rhs._data)), _length(rhs._length), _capacity(rhs._capacity) {
			rhs._data = nullptr;
			rhs._length = 0;
			rhs._capacity = 0;
		}
		PEFF_FORCEINLINE ~DynArray() {
			_clear();
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			verifyAlloc(_allocator.get(), rhs._allocator.get());
			_clear();

			_allocator = rhs._allocator;
			_data = rhs._data;
			_length = rhs._length;
			_capacity = rhs._capacity;

			rhs._data = nullptr;
			rhs._length = 0;
			rhs._capacity = 0;

			return *this;
		}

		PEFF_FORCEINLINE size_t size() {
			return _length;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool resize(size_t length) {
			return _resize<true>(length, true);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool resizeWithoutShrink(size_t length) {
			return _resize<true>(length, false);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool resizeUninitialized(size_t length) {
			return _resize<false>(length, true);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool resizeWithoutShrinkUninitialized(size_t length) {
			return _resize<false>(length, false);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build(const ThisType &rhs) {
			clear();

			if (!resizeUninitialized(rhs.size())) {
				return false;
			}

			size_t i = 0;

			peff::ScopeGuard destructGuard([this, &i]() noexcept {
				if constexpr (!std::is_trivially_destructible_v<T>) {
					for (size_t j = 0; j < i; ++j) {
						std::destroy_at<T>(&_data[j]);
					}
				}
			});

			while (i < _length) {
				peff::constructAt<T>(&_data[i], rhs._data[i]);
				++i;
			}

			destructGuard.release();

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build(const std::initializer_list<T> &rhs) {
			clear();

			if (!resizeUninitialized(rhs.size())) {
				return false;
			}

			size_t i = 0;

			peff::ScopeGuard destructGuard([this, &i]() noexcept {
				if constexpr (!std::is_trivially_destructible_v<T>) {
					for (size_t j = 0; j < i; ++j) {
						std::destroy_at<T>(&_data[j]);
					}
				}
			});

			for(const auto &i : rhs) {
				peff::constructAt<T>(&_data[i], i);
				++i;
			}

			destructGuard.release();

			return true;
		}

		PEFF_FORCEINLINE void clear() {
			_clear();
		}

		PEFF_FORCEINLINE T &at(size_t index) {
			assert(index < _length);
			return _data[index];
		}

		PEFF_FORCEINLINE const T &at(size_t index) const {
			assert(index < _length);
			return _data[index];
		}

		PEFF_FORCEINLINE size_t size() const {
			return _length;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertRangeInitialized(size_t index, size_t length) {
			if (!_insertRange<true>(index, length))
				return false;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertRangeUninitialized(size_t index, size_t length) {
			if (!_insertRange<true>(index, length))
				return false;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(size_t index, T &&data) {
			T *gap = (T *)_insertRange<false>(index, 1);

			if (!gap)
				return false;

			constructAt<T>(gap, std::move(data));

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushFront(T &&data) {
			return insert(0, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushBack(T &&data) {
			return insert(_length, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE T &front() {
			return at(0);
		}

		[[nodiscard]] PEFF_FORCEINLINE const T &front() const {
			return at(0);
		}

		[[nodiscard]] PEFF_FORCEINLINE T &back() {
			return at(_length - 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE const T &back() const {
			return at(_length - 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popBack() {
			return resizeUninitialized(_length - 1);
		}

		PEFF_FORCEINLINE void popBackWithoutShrink() {
			bool unused = resizeUninitialized(_length - 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popFront() {
			_moveData(_data, _data + 1, _length - 1);
			return resize(_length - 1);
		}

		PEFF_FORCEINLINE void popFrontWithoutShrink() {
			_moveData(_data, _data + 1, _length - 1);
			bool unused = resize(_length - 1);
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator.get();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) {
			verifyReplaceable(_allocator.get(), rhs);

			_allocator = rhs;
		}

		PEFF_FORCEINLINE T *data() {
			return _data;
		}

		PEFF_FORCEINLINE const T *data() const {
			return _data;
		}

		PEFF_FORCEINLINE Iterator begin() {
			return _data;
		}

		PEFF_FORCEINLINE Iterator end() {
			return _data + _length;
		}

		PEFF_FORCEINLINE ConstIterator begin() const {
			return _data;
		}

		PEFF_FORCEINLINE ConstIterator end() const {
			return _data + _length;
		}
	};
}

#endif
