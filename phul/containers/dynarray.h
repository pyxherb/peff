#ifndef _PHUL_UTILS_DYNARRAY_H_
#define _PHUL_UTILS_DYNARRAY_H_

#include "basedefs.h"
#include <memory_resource>
#include <cassert>
#include <functional>
#include <phul/base/allocator.h>

namespace phul {
	template <typename T, typename Allocator = StdAlloc>
	class DynArray {
	public:
		T *_data = nullptr;
		size_t _length = 0;
		size_t _capacity = 0;
		Allocator _allocator;

		PHUL_FORCEINLINE static int _checkCapacity(size_t length, size_t capacity) {
			if (length > capacity)
				return 1;
			if (capacity < (length >> 1))
				return -1;
			return 0;
		}

		PHUL_FORCEINLINE void _moveData(T *newData, T *oldData, size_t length) noexcept {
			for (size_t i = 0; i < length; ++i) {
				newData[i] = std::move(oldData[i]);
			}
		}

		PHUL_FORCEINLINE void _moveDataUninitialized(T *newData, T *oldData, size_t length) noexcept {
			for (size_t i = 0; i < length; ++i) {
				new (&newData[i]) T(std::move(oldData[i]));
			}
		}

		PHUL_FORCEINLINE void _constructData(T *newData, size_t length) {
			for (size_t i = 0; i < length; ++i) {
				new (&newData[i]) T();
			}
		}

		PHUL_FORCEINLINE void _expand(
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
		}

		PHUL_FORCEINLINE void _shrink(
			T *newData,
			size_t length) {
			assert(length < _length);

			for (size_t i = length; i < _length; ++i) {
				std::destroy_at<T>(&_data[i]);
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, length);
			}
		}

		PHUL_FORCEINLINE void _resize(size_t length) {
			if (length == _length)
				return;

			if (!length) {
				_clear();
			}

			int capacityStatus = _checkCapacity(length, _capacity);
			if (capacityStatus > 0) {
				size_t newCapacity = _capacity ? (_capacity << 1) : length,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator.alloc(newCapacityTotalSize, sizeof(T));

				if constexpr (std::is_trivial_v<T>) {
					memcpy(newData, _data, sizeof(T) * _length);
				} else {
					ScopeGuard scopeGuard(
						[]() {
							_allocator.release(newData);
						});

					_expand(newData, length);

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator.release(_data);
				_data = newData;
			} else if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1,
					   newCapacityTotalSize = newCapacity * sizeof(T);
				T *newData = (T *)_allocator.alloc(newCapacityTotalSize, sizeof(T));

				if constexpr (std::is_trivial_v<T>) {
					memcpy(newData, _data, sizeof(T) * length);
				} else {
					ScopeGuard scopeGuard(
						[]() {
							_allocator.release(newData);
						});

					_shrink(newData, length);

					scopeGuard.release();
				}

				_capacity = newCapacity;
				_allocator.release(_data);
				_data = newData;
			} else {
				if constexpr (std::is_trivial_v<T>) {
					// No need to do anything.
				} else {
					if (length > _length)
						_expand(_data, length);
					else
						_shrink(_data, length);
				}
			}

			_length = length;
		}

		PHUL_FORCEINLINE void _immediateResize(size_t length) {
			if (length == _length)
				return;

			size_t newTotalSize = length * sizeof(T);
			T *newData = (T *)_allocator.alloc(newTotalSize);

			if constexpr (std::is_trivial_v<T>) {
				memcpy(newData, _data, length * sizeof(T));
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

		PHUL_FORCEINLINE void _clear() {
			if constexpr (!std::is_trivial_v<T>) {
				for (size_t i = 0; i < _length; ++i)
					std::destroy_at<T>(&_data[i]);
			}
			_allocator.release(_data);

			_data = nullptr;
		}

		PHUL_FORCEINLINE void _eraseRange(size_t idxStart, size_t idxEnd) {
			const size_t gapLength = idxEnd - idxStart;
			const size_t postGapLength = _length - idxEnd;
			const size_t newLength = _length - gapLength;

			if constexpr (!std::is_trivial_v<T>) {
				memcpy(&_data[idxStart], &_data[idxEnd], postGapLength * sizeof(T));
			} else {
				for (size_t i = idxStart; i < idxEnd; ++i)
					std::destroy_at<T>(&_data[i]);
				if (idxEnd < _length)
					_moveDataUninitialized(&_data[idxStart], &_data[idxEnd], postGapLength);
			}
			_resize(newLength);
		}
		PHUL_FORCEINLINE void _extractRange(size_t idxStart, size_t idxEnd) {
			const size_t newLength = idxEnd - idxStart;

			if (newLength > idxStart) {
			}
		}

		/// @brief Reserve an area in front of specified index.
		/// @param index Index to be reserved.
		/// @param length Length of space to reserve.
		/// @param construct Determines if to construct objects.
		/// @return Pointer to the reserved area.
		PHUL_CONTAINERS_API T *_reserveSlots(
			size_t index,
			size_t length,
			bool construct) {
			const size_t
				oldLength = _length,
				newLength = _length + length;

			_resize(newLength);

			T *gapStart = &_data[index];

			if (std::is_trivial_v<T>) {
				if (index < oldLength) {
					memcpy(&_data[index + length], gapStart, sizeof(T) * length);
				}
			} else {
				if (index < oldLength) {
					_moveDataUninitialized(
						&_data[(index + length)],
						gapStart,
						length);
				}

				if (construct) {
					_constructData(gapStart, length);
				}
			}

			return gapStart;
		}

	public:
		DynArray() {
		}
		~DynArray() {
			_clear();
		}

		PHUL_FORCEINLINE size_t getSize() {
			return _length;
		}

		PHUL_FORCEINLINE void resize(size_t length) {
			_resize(length);
		}

		PHUL_FORCEINLINE void clear() {
			_clear();
		}

		PHUL_FORCEINLINE T &at(size_t index) {
			return _data[index];
		}

		PHUL_FORCEINLINE T &at(size_t index) const {
			return _data[index];
		}

		PHUL_FORCEINLINE size_t getSize() const {
			return _length;
		}

		PHUL_FORCEINLINE void reserveSlots(size_t index, size_t length) {
			_reserveSlots(index, length, true);
		}

		PHUL_FORCEINLINE void insertFront(size_t index, const T &data) {
			T *gap = (T *)_reserveSlots(index, 1, false);

			if constexpr (std::is_trivial_v<T>) {
				*gap = data;
			} else {
				new (gap) T();
			}
		}

		PHUL_FORCEINLINE void insertFront(size_t index, T &&data) {
			T *gap = (T *)_reserveSlots(index, 1, false);

			if constexpr (std::is_trivial_v<T>) {
				*gap = data;
			} else {
				new (gap) T(data);
			}
		}
	};
}

#endif
