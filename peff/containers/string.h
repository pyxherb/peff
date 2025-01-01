#ifndef _PEFF_CONTAINERS_STRING_H_
#define _PEFF_CONTAINERS_STRING_H_

#include "basedefs.h"
#include <peff/utils/scope_guard.h>
#include <peff/base/allocator.h>

namespace peff {
	class String {
	public:
		char *_data = nullptr;
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

		PEFF_FORCEINLINE void _moveData(char *newData, char *oldData, size_t length) noexcept {
			memmove(newData, oldData, sizeof(char) * length);
		}

		PEFF_FORCEINLINE void _moveDataUninitialized(char *newData, char *oldData, size_t length) noexcept {
			memmove(newData, oldData, sizeof(char) * length);
		}

		PEFF_FORCEINLINE void _expand(
			char *newData,
			size_t length) {
			assert(length > _length);

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, _length);
			}
		}

		PEFF_FORCEINLINE void _shrink(
			char *newData,
			size_t length) noexcept {
			assert(length < _length);

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
					   newCapacityTotalSize = newCapacity * sizeof(char);
				char *newData = (char *)_allocator->alloc(newCapacityTotalSize, sizeof(char));

				if (!newData)
					return false;

				memmove(newData, _data, sizeof(char) * _length);

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else if (capacityStatus < 0) {
				size_t newCapacity = _capacity >> 1,
					   newCapacityTotalSize = newCapacity * sizeof(char);
				char *newData = (char *)_allocator->alloc(newCapacityTotalSize, sizeof(char));

				if (!newData)
					return false;

				memmove(newData, _data, sizeof(char) * length);

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else {
			}

			_length = length;
			return true;
		}

		PEFF_FORCEINLINE void _immediateResize(size_t length) {
			if (length == _length)
				return;

			size_t newTotalSize = length * sizeof(char);
			char *newData = (char *)_allocator->alloc(newTotalSize);

			memmove(newData, _data, length * sizeof(char));

			_data = newData;
			_capacity = length;
		}

		PEFF_FORCEINLINE void _clear() {
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
					   newCapacityTotalSize = newCapacity * sizeof(char);
				char *newData = (char *)_allocator->alloc(newCapacityTotalSize, sizeof(char));

				if (!newData)
					return false;

				memmove(newData, _data, sizeof(char) * idxStart);
				memmove(newData + idxStart, _data + idxEnd, sizeof(char) * postGapLength);

				_capacity = newCapacity;
				_allocator->release(_data);
				_data = newData;
			} else {
				memmove(&_data[idxStart], &_data[idxEnd], postGapLength * sizeof(char));
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
		PEFF_FORCEINLINE char *_reserveSlots(
			size_t index,
			size_t length,
			bool construct) {
			const size_t
				oldLength = _length,
				newLength = _length + length;

			_resize(newLength);

			char *gapStart = &_data[index];

			if (std::is_trivially_move_assignable_v<char>) {
				if (index < oldLength) {
					memmove(&_data[index + length], gapStart, sizeof(char) * (oldLength - index));
				}
			} else {
				if (index < oldLength) {
					_moveDataUninitialized(
						&_data[index + length + 1],
						gapStart,
						oldLength - index);
				}
			}

			return gapStart;
		}

	public:
		String(Alloc *allocator = getDefaultAlloc()) : _allocator(allocator) {
		}
		~String() {
			_clear();
		}

		PEFF_FORCEINLINE size_t size() {
			return _length;
		}

		PEFF_FORCEINLINE void resize(size_t length) {
			_resize(length);
		}

		PEFF_FORCEINLINE void clear() {
			_clear();
		}

		PEFF_FORCEINLINE char &at(size_t index) {
			return _data[index];
		}

		PEFF_FORCEINLINE const char &at(size_t index) const {
			return _data[index];
		}

		PEFF_FORCEINLINE size_t size() const {
			return _length;
		}

		PEFF_FORCEINLINE void reserveSlots(size_t index, size_t length) {
			_reserveSlots(index, length, true);
		}

		PEFF_FORCEINLINE void insert(size_t index, char data) {
			char *gap = (char *)_reserveSlots(index, 1, false);

			*gap = data;
		}
	};
}

#endif
