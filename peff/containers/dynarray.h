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

		PEFF_FORCEINLINE static size_t _get_grown_capacity(size_t length, size_t old_capacity) {
			if (!old_capacity)
				return length;

			size_t new_capacity = old_capacity + (old_capacity >> 1);

			if (new_capacity < length)
				return length;

			return new_capacity;
		}

		PEFF_FORCEINLINE static size_t _get_shrinked_capacity(size_t length, size_t old_capacity) {
			if (!old_capacity)
				return length;

			size_t new_capacity = old_capacity >> 1;

			if (new_capacity > length)
				return length;

			return new_capacity;
		}

		PEFF_FORCEINLINE static int _check_capacity(size_t length, size_t capacity) {
			if (length > capacity)
				return 1;
			if (capacity < (length >> 1))
				return -1;
			return 0;
		}

		PEFF_FORCEINLINE void _move_data(T *new_data, T *old_data, size_t length) noexcept {
			if constexpr (std::is_trivially_move_assignable_v<T>) {
				memmove(new_data, old_data, sizeof(T) * length);
			} else {
				if (new_data + length <= old_data) {
					for (size_t i = 0; i < length; ++i) {
						move_assign_or_move_construct<T>(new_data[i], std::move(old_data[i]));
					}
				} else {
					for (size_t i = length; i > 0; --i) {
						move_assign_or_move_construct<T>(new_data[i - 1], std::move(old_data[i - 1]));
					}
				}
			}
		}

		PEFF_FORCEINLINE void move_data_uninit(T *new_data, T *old_data, size_t length) noexcept {
			if constexpr (std::is_trivially_move_constructible_v<T>) {
				memmove(new_data, old_data, sizeof(T) * length);
			} else {
				if (new_data + length < old_data) {
					for (size_t i = 0; i < length; ++i) {
						construct_at<T>(&new_data[i], std::move(old_data[i]));
					}
				} else {
					for (size_t i = length; i > 0; --i) {
						construct_at<T>(&new_data[i - 1], std::move(old_data[i - 1]));
					}
				}
			}
		}

		PEFF_FORCEINLINE void _construct_data(T *new_data, size_t length) {
			if constexpr (!std::is_trivially_constructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					construct_at<T>(&new_data[i]);
				}
			}
		}

		PEFF_FORCEINLINE void _destruct_data(T *original_data, size_t length) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (size_t i = 0; i < length; ++i) {
					std::destroy_at<T>(&original_data[i]);
				}
			}
		}

		template <bool construct>
		PEFF_FORCEINLINE void _expand_to(
			T *new_data,
			size_t length) {
			assert(length > _length);

			if constexpr (construct) {
				static_assert(std::is_constructible_v<T>, "The type is not defaultly constructible");
				if constexpr (!std::is_trivially_constructible_v<T>) {
					// Because construction of new objects may throw exceptions,
					// we choose to construct the new objects first.
					size_t idx_last_constructed_object;
					ScopeGuard scope_guard(
						[this, &idx_last_constructed_object, new_data]() noexcept {
							for (size_t i = _length;
								i < idx_last_constructed_object;
								++i) {
								std::destroy_at<T>(&new_data[i]);
							}
						});

					for (size_t i = _length;
						i < length;
						++i) {
						idx_last_constructed_object = i;
						construct_at<T>(&new_data[i]);
					}

					scope_guard.release();
				}
			}

			if (new_data != _data) {
				if (_data)
					move_data_uninit(new_data, _data, _length);
			}
		}

		PEFF_FORCEINLINE void _shrink(
			T *new_data,
			size_t length) noexcept {
			assert(length < _length);

			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (size_t i = length; i < _length; ++i) {
					std::destroy_at<T>(&_data[i]);
				}
			}

			if (new_data != _data) {
				if (_data)
					move_data_uninit(new_data, _data, length);
			}
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _grow_capacity(size_t length, size_t new_capacity) {
			assert(new_capacity > _length);
			assert(length > _length);

			size_t new_capacity_total_size = new_capacity * sizeof(T);
			T *new_data;
			bool clear_old_data = true;

			if constexpr (std::is_trivially_move_assignable_v<T>) {
				if (_data) {
					if (!(new_data = (T *)_allocator->realloc(_data, sizeof(T) * _capacity, alignof(T), new_capacity_total_size, alignof(T))))
						return false;
					clear_old_data = false;
				} else {
					if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
						return false;
				}
			} else {
				if (_data) {
					if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
						return false;
					if constexpr (std::is_nothrow_constructible_v<T>) {
						_expand_to<construct>(new_data, length);
					} else {
						ScopeGuard scope_guard(
							[this, new_capacity_total_size, new_data]() noexcept {
								_allocator->release(new_data, new_capacity_total_size, alignof(T));
							});
						_expand_to<construct>(new_data, length);
						scope_guard.release();
					}
				} else {
					if constexpr (std::is_nothrow_constructible_v<T>) {
						if ((new_data = (T *)_allocator->realloc_in_place(
								 _data,
								 sizeof(T) * _capacity, alignof(T),
								 new_capacity_total_size, alignof(T)))) {
							_expand_to<construct>(new_data, length);
						} else {
							if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
								return false;
							ScopeGuard scope_guard(
								[this, new_capacity_total_size, new_data]() noexcept {
									_allocator->release(new_data, new_capacity_total_size, alignof(T));
								});
							_expand_to<construct>(new_data, length);
							scope_guard.release();
						}
					} else {
						if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
							return false;
						ScopeGuard scope_guard(
							[this, new_capacity_total_size, new_data]() noexcept {
								_allocator->release(new_data, new_capacity_total_size, alignof(T));
							});
						_expand_to<construct>(new_data, length);
						scope_guard.release();
					}
				}
			}

			if (clear_old_data)
				_clear();
			_capacity = new_capacity;
			_data = new_data;
			_length = length;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _shrink_capacity(size_t length, size_t new_capacity) {
			assert(length <= _length);
			assert(new_capacity <= _capacity);

			size_t new_capacity_total_size = new_capacity * sizeof(T);
			T *new_data;
			bool clear_old_data = true;

			if constexpr (std::is_trivially_move_assignable_v<T>) {
				if (_data) {
					if (!(new_data = (T *)_allocator->realloc(_data, sizeof(T) * _capacity, alignof(T), new_capacity_total_size, alignof(T))))
						return false;
					clear_old_data = false;
				} else {
					if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
						return false;
				}
			} else {
				if (!(new_data = (T *)_allocator->alloc(new_capacity_total_size, alignof(T))))
					return false;
			}

			if (!new_data) {
				return false;
			}

			if constexpr (std::is_trivially_move_assignable_v<T>) {
			} else {
				_shrink(new_data, length);
			}

			if (clear_old_data)
				_clear();
			_capacity = new_capacity;
			_data = new_data;
			_length = length;
			return true;
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _auto_grow_capacity(size_t length) {
			if (length > _capacity) {
				if (!_grow_capacity<construct>(length, _get_grown_capacity(length, _capacity)))
					return false;
			}
			return true;
		}

		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE bool _auto_shrink_capacity(size_t length) {
			if (!length) {
				_clear();
				return true;
			}
			if (length < (_capacity >> 1)) {
				if (!_shrink_capacity(length, _get_shrinked_capacity(length, _capacity)))
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

		[[nodiscard]] PEFF_FORCEINLINE bool erase_range_and_shrink(size_t idx_start, size_t idx_end) {
			assert(idx_start < _length);
			assert(idx_end <= _length);

			const size_t gap_length = idx_end - idx_start;
			const size_t post_gap_length = _length - idx_end;
			const size_t new_length = _length - gap_length;

			if (post_gap_length < gap_length) {
				_destruct_data(_data + idx_start + post_gap_length, (gap_length - post_gap_length));
			}
			_move_data(_data + idx_start, _data + idx_end, _length - idx_end);

			return _shrink_capacity(new_length, new_length);
		}

		PEFF_FORCEINLINE void erase_range(size_t idx_start, size_t idx_end) {
			assert(idx_start < _length);
			assert(idx_end <= _length);

			const size_t gap_length = idx_end - idx_start;
			const size_t post_gap_length = _length - idx_end;
			const size_t new_length = _length - gap_length;

			if (post_gap_length < gap_length) {
				_destruct_data(_data + idx_start + post_gap_length, (gap_length - post_gap_length));
			}
			_move_data(_data + idx_start, _data + idx_end, _length - idx_end);

			_length = new_length;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool extract_range_and_shrink(size_t idx_start, size_t idx_end) {
			const size_t new_length = idx_end - idx_start;

			if (new_length == _length)
				return true;

			return _shrink_capacity(new_length, new_length);
		}

		PEFF_FORCEINLINE void extract_range(size_t idx_start, size_t idx_end) {
			const size_t new_length = idx_end - idx_start;

			if (new_length == _length)
				return;

			if (!new_length) {
				clear();
				return;
			}

			if (idx_start) {
				_move_data(_data, _data + idx_start, new_length);
				bool result = resize(new_length);
				assert(result);
			} else {
				bool result = resize(new_length);
				assert(result);
			}
		}
		/// @brief Reserve an area in front of specified index.
		/// @param index Index to be reserved.
		/// @param length Length of space to reserve.
		/// @param construct Determines if to construct objects.
		/// @return Pointer to the reserved area.
		template <bool construct>
		[[nodiscard]] PEFF_FORCEINLINE T *_insert_range(
			size_t index,
			size_t length) {
			const size_t
				old_length = _length,
				new_length = _length + length;

			if (!_auto_grow_capacity<construct>(new_length))
				return nullptr;

			T *gap_start = &_data[index];

			if (std::is_trivially_move_assignable_v<T>) {
				if (index < old_length) {
					memmove(&_data[index + length], gap_start, sizeof(T) * (old_length - index));
				}
			} else {
				if (index < old_length) {
					move_data_uninit(
						&_data[index + length],
						gap_start,
						old_length - index);
				}

				if constexpr (construct) {
					_construct_data(gap_start, length);
				}
			}

			_length = new_length;
			return gap_start;
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
			verify_allocator(_allocator.get(), rhs._allocator.get());
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
		[[nodiscard]] PEFF_FORCEINLINE bool resize_and_shrink(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_grow_capacity<true>(length, length))
						return false;
				} else {
					if (!_shrink_capacity(length, length))
						return false;
					_expand_to<true>(_data, length);
				}
			} else if (length < _length) {
				if (!_shrink_capacity(length, length))
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
					if (!_auto_grow_capacity<true>(length))
						return false;
				} else {
					_expand_to<true>(_data, length);
					_length = length;
				}
			} else if (length < _length) {
				_shrink(_data, length);
				_length = length;
			}
			return true;
		}

		///
		/// @brief Resize and shrink the capacity automatically, but don't initialize the new elements.
		///
		/// @param length New length for resizing.
		/// @return Whether the resizing operation is performed successfully.
		///
		[[nodiscard]] PEFF_FORCEINLINE bool resize_and_shrink_uninit(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_grow_capacity<false>(length, length))
						return false;
				} else {
					if (!_shrink_capacity(length, length))
						return false;
					_expand_to<false>(_data, length);
				}
			} else if (length < _length) {
				if (!_shrink_capacity(length, length))
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
		[[nodiscard]] PEFF_FORCEINLINE bool resize_uninit(size_t length) {
			if (length > _length) {
				if (length > _capacity) {
					if (!_auto_grow_capacity<false>(length))
						return false;
				} else {
					_expand_to<false>(_data, length);
					_length = length;
				}
			} else if (length < _length) {
				_shrink(_data, length);
				_length = length;
			}
			return true;
		}

		///
		/// @brief Shrink the capacity to the dynamic array's length.
		///
		/// @return PEFF_FORCEINLINE
		///
		[[nodiscard]] PEFF_FORCEINLINE bool shrink_to_fit() {
			if (_length > _capacity)
				return _shrink_capacity(_length, _length);
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build(const ThisType &rhs) {
			clear();

			if (!resize_uninit(rhs.size())) {
				return false;
			}

			size_t i = 0;

			peff::ScopeGuard destruct_guard([this, &i]() noexcept {
				if constexpr (!std::is_trivially_destructible_v<T>) {
					for (size_t j = 0; j < i; ++j) {
						std::destroy_at<T>(&_data[j]);
					}
				}
			});

			while (i < _length) {
				peff::construct_at<T>(&_data[i], rhs._data[i]);
				++i;
			}

			destruct_guard.release();

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build(const std::initializer_list<T> &rhs) {
			clear();

			if (!resize_uninit(rhs.size())) {
				return false;
			}

			size_t i = 0;

			peff::ScopeGuard destruct_guard([this, &i]() noexcept {
				if constexpr (!std::is_trivially_destructible_v<T>) {
					for (size_t j = 0; j < i; ++j) {
						std::destroy_at<T>(&_data[j]);
					}
				}
			});

			for (const auto &item : rhs) {
				peff::construct_at<T>(&_data[i], item);
				++i;
			}

			destruct_guard.release();

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build_and_shrink(const ThisType &rhs) {
			if (!build(rhs))
				return false;
			return shrink_to_fit();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool build_and_shrink(const std::initializer_list<T> &rhs) {
			if (!build(rhs))
				return false;
			return shrink_to_fit();
		}

		///
		/// @brief Clear the dynamic array, but don't clear the capacity.
		///
		PEFF_FORCEINLINE void clear() {
			if (!resize_uninit(0))
				std::terminate();
		}

		PEFF_FORCEINLINE void clear_and_shrink() {
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

		[[nodiscard]] PEFF_FORCEINLINE bool insert_range_init(size_t index, size_t length) {
			if (!_insert_range<true>(index, length))
				return false;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert_range_uninit(size_t index, size_t length) {
			if (!_insert_range<true>(index, length))
				return false;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(size_t index, T &&data) {
			T *gap = (T *)_insert_range<false>(index, 1);

			if (!gap)
				return false;

			construct_at<T>(gap, std::move(data));

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool push_front(T &&data) {
			return insert(0, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool push_back(T &&data) {
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

		[[nodiscard]] PEFF_FORCEINLINE bool pop_back_and_shrink() {
			return resize_uninit(_length - 1);
		}

		PEFF_FORCEINLINE void pop_back() {
			bool unused = resize_uninit(_length - 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pop_front_and_shrink() {
			T front_data = std::move(front());
			size_t len = _length - 1;
			_move_data(_data, _data + 1, len);
			if (!_shrink_capacity(len, len)) {
				_move_data(_data + 1, _data, len);
				construct_at(&_data[0], std::move(front_data));
				return false;
			}
			return true;
		}

		PEFF_FORCEINLINE void pop_front() {
			_move_data(_data, _data + 1, _length - 1);
			bool result = resize(_length - 1);
			assert(result);
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator.get();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) {
			verify_replaceable(_allocator.get(), rhs);

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

		PEFF_FORCEINLINE ConstIterator begin_const() const {
			return _data;
		}

		PEFF_FORCEINLINE ConstIterator end_const() const {
			return _data + _length;
		}
	};
}

#endif
