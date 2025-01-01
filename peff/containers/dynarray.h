#ifndef _PEFF_UTILS_DYNARRAY_H_
#define _PEFF_UTILS_DYNARRAY_H_

#include "basedefs.h"
#include <memory_resource>
#include <cassert>
#include <cstring>
#include <functional>
#include <peff/base/allocator.h>

namespace peff {
	template <typename T>
	class DynArray {
	public:
		T *_data = nullptr;
		size_t _length = 0;
		size_t _capacity = 0;
		Alloc *_allocator;

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
				for (size_t i = 0; i < length; ++i) {
					newData[i] = std::move(oldData[i]);
				}
			}
		}

		PEFF_FORCEINLINE void _moveDataUninitialized(T *newData, T *oldData, size_t length) noexcept {
			if constexpr (std::is_trivially_move_constructible_v<T>) {
				memmove(newData, oldData, sizeof(T) * length);
			} else {
				for (size_t i = 0; i < length; ++i) {
					new (&newData[i]) T(std::move(oldData[i]));
				}
			}
		}

		PEFF_FORCEINLINE void _constructData(T *newData, size_t length) {
			if (!std::is_trivially_constructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					new (&newData[i]) T();
				}
			}
		}

		PEFF_FORCEINLINE void _destructData(T *originalData, size_t length) {
			if (!std::is_trivially_destructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					originalData[i].~T();
				}
			}
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _expand(
			T *newData,
			size_t length) {
			assert(length > _length);

			{
				// Because construction of new objects may throw exceptions,
				// we choose to construct the new objects first.
				size_t idxLastConstructedObject;
				ScopeGuard scopeGuard(
					[this, &idxLastConstructedObject, newData]() {
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
					new (&newData[i]) T();
				}

				scopeGuard.release();
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, _length);
			}

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _expandWith(
			T *newData,
			size_t length,
			const T &data) {
			assert(length > _length);

			{
				// Because construction of new objects may throw exceptions,
				// we choose to construct the new objects first.
				size_t idxLastConstructedObject;
				ScopeGuard scopeGuard(
					[this, &idxLastConstructedObject, newData]() {
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
					if (!peff::copy(newData[i], data)) {
						return false;
					}
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

		[[nodiscard]] PEFF_FORCEINLINE bool _resize(size_t length) {
			if (length == _length)
				return true;

			if (!length) {
				_clear();
			}

			int capacityStatus = _checkCapacity(length, _capacity);
			if (capacityStatus > 0) {
				size_t newCapacity = _capacity ? (_capacity << 1) : length,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, sizeof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * _length);
				} else {
					ScopeGuard scopeGuard(
						[this, newData]() {
							_allocator->release(newData);
						});

					if (!_expand(newData, length))
						return false;

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, sizeof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * length);
				} else {
					ScopeGuard scopeGuard(
						[this, newData]() {
							_allocator->release(newData);
						});

					_shrink(newData, length);

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else {
				if (length > _length) {
					if constexpr (!std::is_trivially_constructible_v<T>) {
						if (!_expand(_data, length))
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

		[[nodiscard]] PEFF_FORCEINLINE bool _resizeWith(size_t length, const T& filler) {
			if (length == _length)
				return true;

			if (!length) {
				_clear();
			}

			int capacityStatus = _checkCapacity(length, _capacity);
			if (capacityStatus > 0) {
				size_t newCapacity = _capacity ? (_capacity << 1) : length,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, sizeof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * _length);
				} else {
					ScopeGuard scopeGuard(
						[this, newData]() {
							_allocator->release(newData);
						});

					if (!_expandWith(newData, length, filler)) {
						return false;
					}

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, sizeof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * length);
				} else {
					ScopeGuard scopeGuard(
						[this, newData]() {
							_allocator->release(newData);
						});

					_shrink(newData, length);

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else {
				if (length > _length) {
					if constexpr (!std::is_trivially_constructible_v<T>) {
						if (!_expandWith(_data, length, filler)) {
							return false;
						}
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

		PEFF_FORCEINLINE void _immediateResize(size_t length) {
			if (length == _length)
				return;

			size_t newTotalSize = length * sizeof(T);
			T *newData = (T *)_allocator->alloc(newTotalSize);

			if constexpr (std::is_trivially_move_assignable_v<T>) {
				memmove(newData, _data, length * sizeof(T));
			} else {
				if (length > _length) {
					_expand(newData, length);
				} else {
					_shrink(newData, length);
				}
			}

			_data = newData;
			_capacity = length;
		}

		PEFF_FORCEINLINE void _clear() {
			if constexpr (!std::is_trivial_v<T>) {
				for (size_t i = 0; i < _length; ++i)
					std::destroy_at<T>(&_data[i]);
			}
			_allocator->release(_data);

			_data = nullptr;
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
				T *newData = (T *)_allocator->alloc(newCapacityTotalSize, sizeof(T));

				if (!newData)
					return false;

				if constexpr (std::is_trivially_move_assignable_v<T>) {
					memmove(newData, _data, sizeof(T) * idxStart);
					memmove(newData + idxStart, _data + idxEnd, sizeof(T) * postGapLength);
				} else {
					ScopeGuard scopeGuard(
						[this, newData]() {
							_allocator->release(newData);
						});

					_moveData(newData, _data, idxStart);
					_moveData(newData + idxStart, _data + idxEnd, postGapLength);

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator->release(_data);
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

		PEFF_FORCEINLINE void extractRange(size_t idxStart, size_t idxEnd) {
			const size_t newLength = idxEnd - idxStart;

			if (newLength == _length)
				return;

			if (!newLength) {
				clear();
				return;
			}

			if (idxStart) {
				_moveData(_data, _data + idxStart, newLength);
				_resize(newLength);
			} else {
				_resize(newLength);
			}
		}

		/// @brief Reserve an area in front of specified index.
		/// @param index Index to be reserved.
		/// @param length Length of space to reserve.
		/// @param construct Determines if to construct objects.
		/// @return Pointer to the reserved area.
		PEFF_FORCEINLINE T *_reserveSlots(
			size_t index,
			size_t length,
			bool construct) {
			const size_t
				oldLength = _length,
				newLength = _length + length;

			_resize(newLength);

			T *gapStart = &_data[index];

			if (std::is_trivially_move_assignable_v<T>) {
				if (index < oldLength) {
					memmove(&_data[index + length], gapStart, sizeof(T) * (oldLength - index));
				}
			} else {
				if (index < oldLength) {
					_moveDataUninitialized(
						&_data[index + length + 1],
						gapStart,
						oldLength - index);
				}

				if (construct) {
					_constructData(gapStart, length);
				}
			}

			return gapStart;
		}

	public:
		DynArray(Alloc *allocator = getDefaultAlloc()) : _allocator(allocator) {
		}
		~DynArray() {
			_clear();
		}

		PEFF_FORCEINLINE size_t size() {
			return _length;
		}

		PEFF_FORCEINLINE bool resize(size_t length) {
			return _resize(length);
		}

		PEFF_FORCEINLINE bool resizeWith(size_t length, const T &filler) {
			return _resizeWith(length, filler);
		}

		PEFF_FORCEINLINE void clear() {
			_clear();
		}

		PEFF_FORCEINLINE T &at(size_t index) {
			return _data[index];
		}

		PEFF_FORCEINLINE const T &at(size_t index) const {
			return _data[index];
		}

		PEFF_FORCEINLINE size_t size() const {
			return _length;
		}

		PEFF_FORCEINLINE void reserveSlots(size_t index, size_t length) {
			_reserveSlots(index, length, true);
		}

		PEFF_FORCEINLINE void insert(size_t index, const T &data) {
			T *gap = (T *)_reserveSlots(index, 1, false);

			if constexpr (std::is_trivially_copyable_v<T>) {
				*gap = data;
			} else {
				new (gap) T();
			}
		}

		PEFF_FORCEINLINE void insert(size_t index, T &&data) {
			T *gap = (T *)_reserveSlots(index, 1, false);

			if constexpr (std::is_trivially_copyable_v<T>) {
				*gap = data;
			} else {
				new (gap) T(data);
			}
		}

		PEFF_FORCEINLINE Alloc* allocator() const {
			return _allocator;
		}
	};
}

#endif
