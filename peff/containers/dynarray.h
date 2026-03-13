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

		PEFF_FORCEINLINE static size_t _getGrownCapacity(size_t length, size_t oldCapacity) {
			if (!oldCapacity)
				return length;

			size_t newCapacity = oldCapacity + (oldCapacity >> 1);

			if (newCapacity < length)
				return length;

			return newCapacity;
		}

		PEFF_FORCEINLINE static size_t _getShrinkedCapacity(size_t length, size_t oldCapacity) {
			if (!oldCapacity)
				return length;

			size_t newCapacity = oldCapacity >> 1;

			if (newCapacity > length)
				return length;

			return newCapacity;
		}

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
				if (newData + length <= oldData) {
					for (size_t i = 0; i < length; ++i) {
						moveAssignOrMoveConstruct<T>(newData[i], std::move(oldData[i]));
					}
				} else {
					for (size_t i = length; i > 0; --i) {
						moveAssignOrMoveConstruct<T>(newData[i - 1], std::move(oldData[i - 1]));
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
		PEFF_FORCEINLINE void _expandTo(
			T *newData,
			size_t length) {
			assert(length > _length);

			if constexpr (construct) {
				static_assert(std::is_constructible_v<T>, "The type is not defaultly constructible");
				if constexpr (!std::is_trivially_constructible_v<T>) {
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
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, _length);
			}
		}

		PEFF_FORCEINLINE void _shrink(
			T *newData,
			size_t length) noexcept {
			assert(length < _length);

			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (size_t i = length; i < _length; ++i) {
					std::destroy_at<T>(&_data[i]);
				}
			}

			if (newData != _data) {
				if (_data)
					_moveDataUninitialized(newData, _data, length);
			}
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _growCapacity(size_t length, size_t newCapacity) {
			assert(newCapacity > _length);
			assert(length > _length);

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
				if (_data) {
					if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
						return false;
					if constexpr (std::is_nothrow_constructible_v<T>) {
						_expandTo<construct>(newData, length);
					} else {
						ScopeGuard scopeGuard(
							[this, newCapacityTotalSize, newData]() noexcept {
								_allocator->release(newData, newCapacityTotalSize, alignof(T));
							});
						_expandTo<construct>(newData, length);
						scopeGuard.release();
					}
				} else {
					if constexpr (std::is_nothrow_constructible_v<T>) {
						if ((newData = (T *)_allocator->reallocInPlace(
								 _data,
								 sizeof(T) * _capacity, alignof(T),
								 newCapacityTotalSize, alignof(T)))) {
							_expandTo<construct>(newData, length);
						} else {
							if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
								return false;
							ScopeGuard scopeGuard(
								[this, newCapacityTotalSize, newData]() noexcept {
									_allocator->release(newData, newCapacityTotalSize, alignof(T));
								});
							_expandTo<construct>(newData, length);
							scopeGuard.release();
						}
					} else {
						if (!(newData = (T *)_allocator->alloc(newCapacityTotalSize, alignof(T))))
							return false;
						ScopeGuard scopeGuard(
							[this, newCapacityTotalSize, newData]() noexcept {
								_allocator->release(newData, newCapacityTotalSize, alignof(T));
							});
						_expandTo<construct>(newData, length);
						scopeGuard.release();
					}
				}
			}

			if (clearOldData)
				_clear();
			_capacity = newCapacity;
			_data = newData;
			_length = length;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _shrinkCapacity(size_t length, size_t newCapacity) {
			assert(length <= _length);
			assert(newCapacity <= _capacity);

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
				return false;
			}

			if constexpr (std::is_trivially_move_assignable_v<T>) {
			} else {
				_shrink(newData, length);
			}

			if (clearOldData)
				_clear();
			_capacity = newCapacity;
			_data = newData;
			_length = length;
			return true;
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _autoGrowCapacity(size_t length) {
			if (length > _capacity) {
				if (!_growCapacity<construct>(length, _getGrownCapacity(length, _capacity)))
					return false;
			}
			return true;
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _autoShrinkCapacity(size_t length) {
			if (!length) {
				_clear();
				return true;
			}
			if (length < (_capacity >> 1)) {
				if (!_shrinkCapacity(length, _getShrinkedCapacity(length, _capacity)))
					return false;
			}
			return true;
		}

		///
		/// @brief Internal method for clearing the dynamic array, the capacity will be released immediately.
		///
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

		[[nodiscard]] PEFF_FORCEINLINE bool eraseRangeAndShrink(size_t idxStart, size_t idxEnd) {
			assert(idxStart < _length);
			assert(idxEnd <= _length);

			const size_t gapLength = idxEnd - idxStart;
			const size_t postGapLength = _length - idxEnd;
			const size_t newLength = _length - gapLength;

			if (postGapLength < gapLength) {
				_destructData(_data + idxStart + postGapLength, (gapLength - postGapLength));
			}
			_moveData(_data + idxStart, _data + idxEnd, _length - idxEnd);

			return _shrinkCapacity(newLength, newLength);
		}

		PEFF_FORCEINLINE void eraseRange(size_t idxStart, size_t idxEnd) {
			assert(idxStart < _length);
			assert(idxEnd <= _length);

			const size_t gapLength = idxEnd - idxStart;
			const size_t postGapLength = _length - idxEnd;
			const size_t newLength = _length - gapLength;

			if (postGapLength < gapLength) {
				_destructData(_data + idxStart + postGapLength, (gapLength - postGapLength));
			}
			_moveData(_data + idxStart, _data + idxEnd, _length - idxEnd);

			_length = newLength;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool extractRangeAndShrink(size_t idxStart, size_t idxEnd) {
			const size_t newLength = idxEnd - idxStart;

			if (newLength == _length)
				return true;

			return _shrinkCapacity(newLength, newLength);
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
				bool result = resize(newLength);
				assert(result);
			} else {
				bool result = resize(newLength);
				assert(result);
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

			if (!_autoGrowCapacity<construct>(newLength))
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

			_length = newLength;
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

		PEFF_FORCEINLINE size_t capacity() {
			return _capacity;
		}

		///
		/// @brief Resize and shrink the capacity automatically.
		///
		/// @param length New length for resizing.
		/// @return Whether the resizing operation is performed successfully.
		///
		/// @note The capacity will be shrinked if the construction is failed.
		///
		[[nodiscard]] PEFF_FORCEINLINE bool resizeAndShrink(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_growCapacity<true>(length, length))
						return false;
				} else {
					if (!_shrinkCapacity(length, length))
						return false;
					_expandTo<true>(_data, length);
				}
			} else if (length < _length) {
				if (!_shrinkCapacity(length, length))
					return false;
			}
			return true;
		}

		///
		/// @brief Resize, but don't shrink the capacity.
		///
		/// @param length New length for resizing.
		/// @return Whether the resizing operation is performed successfully.
		///
		[[nodiscard]] PEFF_FORCEINLINE bool resize(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_autoGrowCapacity<true>(length))
						return false;
				} else {
					_expandTo<true>(_data, length);
				}
			} else if (length < _length) {
				_shrink(_data, length);
			}
			return true;
		}

		///
		/// @brief Resize and shrink the capacity automatically, but don't initialize the new elements.
		///
		/// @param length New length for resizing.
		/// @return Whether the resizing operation is performed successfully.
		///
		[[nodiscard]] PEFF_FORCEINLINE bool resizeAndShrinkUninitialized(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_growCapacity<false>(length, length))
						return false;
				} else {
					if (!_shrinkCapacity(length, length))
						return false;
					_expandTo<false>(_data, length);
				}
			} else if (length < _length) {
				if (!_shrinkCapacity(length, length))
					return false;
			}
			return true;
		}

		///
		/// @brief Resize, but don't shrink the capacity and don't initialize the new elements.
		///
		/// @param length New length for resizing.
		/// @return Whether the resizing operation is performed successfully.
		///
		[[nodiscard]] PEFF_FORCEINLINE bool resizeUninitialized(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_autoGrowCapacity<false>(length))
						return false;
				} else {
					_expandTo<false>(_data, length);
				}
			} else if (length < _length) {
				_shrink(_data, length);
			}
			return true;
		}

		///
		/// @brief Shrink the capacity to the dynamic array's length.
		///
		/// @return PEFF_FORCEINLINE
		///
		[[nodiscard]] PEFF_FORCEINLINE bool shrinkToFit() {
			if(_length > _capacity)
				return _shrinkCapacity(_length, _length);
			return true;
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

			for (const auto &item : rhs) {
				peff::constructAt<T>(&_data[i], item);
				++i;
			}

			destructGuard.release();

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool buildAndShrink(const ThisType &rhs) {
			if (!build(rhs))
				return false;
			return shrinkToFit();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool buildAndShrink(const std::initializer_list<T> &rhs) {
			if (!build(rhs))
				return false;
			return shrinkToFit();
		}

		///
		/// @brief Clear the dynamic array, but don't clear the capacity.
		///
		PEFF_FORCEINLINE void clear() {
			if (!resizeUninitialized(0))
				std::terminate();
		}

		PEFF_FORCEINLINE void clearAndShrink() {
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

		[[nodiscard]] PEFF_FORCEINLINE bool popBackAndShrink() {
			return resizeUninitialized(_length - 1);
		}

		PEFF_FORCEINLINE void popBack() {
			bool unused = resizeUninitialized(_length - 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popFrontAndShrink() {
			T frontData = std::move(front());
			size_t len = _length - 1;
			_moveData(_data, _data + 1, len);
			if (!_shrinkCapacity(len, len)) {
				_moveData(_data + 1, _data, len);
				constructAt(&_data[0], std::move(frontData));
				return false;
			}
			return true;
		}

		PEFF_FORCEINLINE void popFront() {
			_moveData(_data, _data + 1, _length - 1);
			bool result = resize(_length - 1);
			assert(result);
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

		PEFF_FORCEINLINE ConstIterator beginConst() const {
			return _data;
		}

		PEFF_FORCEINLINE ConstIterator endConst() const {
			return _data + _length;
		}
	};
}

#endif
