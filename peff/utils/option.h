#ifndef _PEFF_UTILS_OPTION_H_
#define _PEFF_UTILS_OPTION_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace peff {
	/// @brief The type of the empty case of a peff::Option.
	struct NullOption {
	};

	/// @brief The empty value, used for constructing or setting an empty peff::Option.
	constexpr static NullOption NULLOPT;

	/// @brief The type used for containing an optional value or @c peff::NULLOPT.
	/// @tparam T Type of the optional value.
	template <typename T>
	class Option final {
	private:
		alignas(T) char _data[sizeof(T)];
		bool _has_value = false;

		/// @brief Get a pointer to the raw storage, which may not contain any value.
		/// @return The pointer to the raw storage.
		[[nodiscard]] PEFF_FORCEINLINE T *_raw_ptr() noexcept {
			return reinterpret_cast<T *>(_data);
		}

		/// @brief Get a pointer to the stored value.
		/// @pre The instance contains a value.
		/// @return The pointer to the stored value.
		[[nodiscard]] PEFF_FORCEINLINE T *_value_ptr() noexcept {
			return std::launder(reinterpret_cast<T *>(_data));
		}

		/// @brief Get a pointer to the stored value.
		/// @pre The instance contains a value.
		/// @return The pointer to the stored value.
		[[nodiscard]] PEFF_FORCEINLINE const T *_value_ptr() const noexcept {
			return std::launder(reinterpret_cast<const T *>(_data));
		}

		/// @brief Check whether @p data refers to the value stored in this instance.
		/// @param data The value to be checked.
		/// @return @c true if @p data is the stored value, @c false otherwise.
		[[nodiscard]] PEFF_FORCEINLINE bool _is_stored(const T &data) const noexcept {
			return _has_value &&
				   static_cast<const void *>(std::addressof(data)) ==
					   static_cast<const void *>(_data);
		}

		/// @brief Move the value out of @p rhs, destroying it in @p rhs.
		/// @pre This instance is empty and @p rhs is not this instance.
		/// @param rhs The instance to take the value from.
		PEFF_FORCEINLINE void _take_value_from(Option<T> &rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if (rhs._has_value) {
				peff::construct_at<T>(_raw_ptr(), std::move(*rhs._value_ptr()));
				_has_value = true;
				rhs.reset();
			}
		}

		/// @brief Move the stored value out of this instance and destroy it.
		/// @pre The instance contains a value.
		/// @return The extracted value.
		[[nodiscard]] PEFF_FORCEINLINE T _extract() noexcept(std::is_nothrow_move_constructible_v<T>) {
			assert(_has_value);

			T data(std::move(*_value_ptr()));
			reset();
			return data;
		}

	public:
		using value_type = T;

		static_assert(std::is_move_constructible_v<T>, "The type must be move constructible");

		/// @brief Destroy the stored value if there is one, making the instance empty.
		PEFF_FORCEINLINE void reset() noexcept {
			if (_has_value) {
				if constexpr (std::is_destructible_v<T>)
					peff::destroy_at<T>(_value_ptr());
				_has_value = false;
			}
		}

		/// @brief Set the stored value by moving @p data into it.
		/// @note Setting the stored value from itself does nothing.
		/// @param data The value to be stored.
		PEFF_FORCEINLINE void set_value(T &&data) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if (_is_stored(data))
				return;

			reset();
			peff::construct_at<T>(_raw_ptr(), std::move(data));
			_has_value = true;
		}

		/// @brief Set the stored value by copying @p data into it.
		/// @note Setting the stored value from itself does nothing.
		/// @param data The value to be stored.
		PEFF_FORCEINLINE void set_value(const T &data) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			if (_is_stored(data))
				return;

			reset();
			peff::construct_at<T>(_raw_ptr(), data);
			_has_value = true;
		}

		/// @brief Make the instance empty, destroying the stored value if there is one.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		/// @brief Check whether the instance contains a value.
		/// @return @c true if the instance contains a value, @c false otherwise.
		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _has_value;
		}

		/// @brief Check whether the instance contains a value.
		/// @return @c true if the instance contains a value, @c false otherwise.
		PEFF_FORCEINLINE operator bool() const noexcept {
			return _has_value;
		}

		/// @brief Get a reference to the stored value.
		/// @pre The instance contains a value.
		/// @return The reference to the stored value.
		PEFF_FORCEINLINE T &value() & noexcept {
			assert(_has_value);
			return *_value_ptr();
		}

		/// @brief Get a reference to the stored value.
		/// @pre The instance contains a value.
		/// @return The reference to the stored value.
		PEFF_FORCEINLINE const T &value() const & noexcept {
			assert(_has_value);
			return *_value_ptr();
		}

		/// @brief Move the stored value out of this instance.
		/// @pre The instance contains a value.
		/// @return The moved value, while the instance becomes empty.
		PEFF_FORCEINLINE T value() && noexcept(std::is_nothrow_move_constructible_v<T>) {
			return _extract();
		}

		/// @brief Move the stored value out of this instance.
		/// @pre The instance contains a value.
		/// @return The moved value, while the instance becomes empty.
		PEFF_FORCEINLINE T unwrap() noexcept(std::is_nothrow_move_constructible_v<T>) {
			return _extract();
		}

		/// @brief Get a reference to the stored value.
		/// @pre The instance contains a value.
		/// @return The reference to the stored value.
		PEFF_FORCEINLINE T &operator*() & noexcept {
			return value();
		}

		/// @brief Get a reference to the stored value.
		/// @pre The instance contains a value.
		/// @return The reference to the stored value.
		PEFF_FORCEINLINE const T &operator*() const & noexcept {
			return value();
		}

		/// @brief Move the stored value out of this instance.
		/// @pre The instance contains a value.
		/// @return The moved value, while the instance becomes empty.
		PEFF_FORCEINLINE T operator*() && noexcept(std::is_nothrow_move_constructible_v<T>) {
			return std::move(*this).value();
		}

		/// @brief Get a pointer to the stored value.
		/// @pre The instance contains a value.
		/// @return The pointer to the stored value.
		PEFF_FORCEINLINE T *operator->() noexcept {
			assert(_has_value);
			return std::addressof(value());
		}

		/// @brief Get a pointer to the stored value.
		/// @pre The instance contains a value.
		/// @return The pointer to the stored value.
		PEFF_FORCEINLINE const T *operator->() const noexcept {
			assert(_has_value);
			return std::addressof(value());
		}

		/// @brief Construct an empty instance.
		PEFF_FORCEINLINE explicit Option() noexcept {
		}

		/// @brief Destroy the stored value if there is one.
		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		/// @brief Construct an instance by copying the value of @p rhs into it.
		/// @note The stored value is copy-constructed in place, so @c T is not required to be copy-assignable.
		/// @param rhs The instance to copy the value from.
		PEFF_FORCEINLINE Option(const Option<T> &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			if (rhs._has_value) {
				peff::construct_at<T>(_raw_ptr(), *rhs._value_ptr());
				_has_value = true;
			}
		}

		/// @brief Construct an instance containing a copy of @p data.
		/// @param data The value to be copied into the instance.
		PEFF_FORCEINLINE Option(const T &data) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(_raw_ptr(), data);
			_has_value = true;
		}

		/// @brief Construct an instance containing @p data moved into it.
		/// @param data The value to be moved into the instance.
		PEFF_FORCEINLINE Option(T &&data) noexcept(std::is_nothrow_move_constructible_v<T>) {
			peff::construct_at<T>(_raw_ptr(), std::move(data));
			_has_value = true;
		}

		/// @brief Construct an empty instance.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		/// @brief Construct an instance by moving the value of @p rhs out of it.
		/// @param rhs The instance to take the value from.
		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			PEFF_ASSERT(&rhs != this, "Self-assignment is not allowed!");
			_take_value_from(rhs);
		}

		/// @brief Copy the value of @p rhs into this instance.
		/// @note Copying an instance into itself does nothing.
		/// @note The old value is destroyed before the copy is constructed, so a copy constructor of @c T
		/// that throws leaves this instance empty instead of propagating an inconsistent state.
		/// @param rhs The instance to copy the value from.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE Option<T> &operator=(const Option<T> &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			if (this != std::addressof(rhs)) {
				reset();
				if (rhs._has_value) {
					peff::construct_at<T>(_raw_ptr(), *rhs._value_ptr());
					_has_value = true;
				}
			}
			return *this;
		}

		/// @brief Move the value of @p rhs out of it into this instance.
		/// @note Moving an instance into itself does nothing.
		/// @param rhs The instance to take the value from.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if (this != std::addressof(rhs)) {
				reset();
				_take_value_from(rhs);
			}
			return *this;
		}

		/// @brief Set the stored value by copying @p rhs into it.
		/// @note Setting the stored value from itself does nothing.
		/// @param rhs The value to be stored.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE Option<T> &operator=(const T &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			set_value(rhs);
			return *this;
		}

		/// @brief Set the stored value by moving @p rhs into it.
		/// @note Setting the stored value from itself does nothing.
		/// @param rhs The value to be stored.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE Option<T> &operator=(T &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			set_value(std::move(rhs));
			return *this;
		}
	};

	/// @brief The type used for containing an optional reference or @c peff::NULLOPT.
	/// @tparam T Type of the referenced value.
	template <typename T>
	class Option<T &> final {
	private:
		using V = T;
		using ThisType = Option<T &>;

		V *_data = nullptr;

		/// @brief Take the referenced value of @p rhs, clearing it in @p rhs.
		/// @param rhs The instance to take the reference from.
		PEFF_FORCEINLINE void _take_value_from(ThisType &rhs) noexcept {
			_data = rhs._data;
			rhs._data = nullptr;
		}

	public:
		using value_type = T &;

		/// @brief Clear the referenced value, making the instance empty.
		PEFF_FORCEINLINE void reset() noexcept {
			_data = nullptr;
		}

		/// @brief Set the referenced value.
		/// @param data The value to be referenced.
		PEFF_FORCEINLINE void set_value_ref(V &data) noexcept {
			_data = std::addressof(data);
		}

		/// @brief Make the instance empty.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		/// @brief Check whether the instance references a value.
		/// @return @c true if the instance references a value, @c false otherwise.
		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _data != nullptr;
		}

		/// @brief Check whether the instance references a value.
		/// @return @c true if the instance references a value, @c false otherwise.
		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data != nullptr;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &value() noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE const V &value() const noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value and make the instance empty.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &unwrap() noexcept {
			assert(has_value());

			V *data = _data;
			_data = nullptr;
			return *data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE const V &operator*() const noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get a pointer to the referenced value.
		/// @pre The instance references a value.
		/// @return The pointer to the referenced value.
		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(has_value());
			return _data;
		}

		/// @brief Get a pointer to the referenced value.
		/// @pre The instance references a value.
		/// @return The pointer to the referenced value.
		PEFF_FORCEINLINE const V *operator->() const noexcept {
			assert(has_value());
			return _data;
		}

		/// @brief Construct an empty instance.
		PEFF_FORCEINLINE explicit Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		/// @brief Construct an instance referencing @p data.
		/// @param data The value to be referenced.
		PEFF_FORCEINLINE Option(V &data) noexcept {
			set_value_ref(data);
		}

		/// @brief Construct an empty instance.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		/// @brief Construct an instance by taking the reference of @p rhs from it.
		/// @param rhs The instance to take the reference from.
		PEFF_FORCEINLINE Option(ThisType &&rhs) noexcept {
			PEFF_ASSERT(&rhs != this, "Self-assignment is not allowed!");
			_take_value_from(rhs);
		}

		/// @brief Take the reference of @p rhs from it into this instance.
		/// @note Taking the reference of an instance into itself does nothing.
		/// @param rhs The instance to take the reference from.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			if (this != std::addressof(rhs))
				_take_value_from(rhs);
			return *this;
		}

		/// @brief Set the referenced value.
		/// @param rhs The value to be referenced.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE ThisType &operator=(V &rhs) noexcept {
			set_value_ref(rhs);
			return *this;
		}
	};

	/// @brief The type used for containing an optional const reference or @c peff::NULLOPT.
	/// @tparam T Type of the referenced value.
	template <typename T>
	class Option<const T &> final {
	private:
		using V = const T;
		using ThisType = Option<const T &>;

		V *_data = nullptr;

		/// @brief Take the referenced value of @p rhs, clearing it in @p rhs.
		/// @param rhs The instance to take the reference from.
		PEFF_FORCEINLINE void _take_value_from(ThisType &rhs) noexcept {
			_data = rhs._data;
			rhs._data = nullptr;
		}

	public:
		using value_type = const T &;

		/// @brief Clear the referenced value, making the instance empty.
		PEFF_FORCEINLINE void reset() noexcept {
			_data = nullptr;
		}

		/// @brief Set the referenced value.
		/// @param data The value to be referenced.
		PEFF_FORCEINLINE void set_value_ref(V &data) noexcept {
			_data = std::addressof(data);
		}

		/// @brief Make the instance empty.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		/// @brief Check whether the instance references a value.
		/// @return @c true if the instance references a value, @c false otherwise.
		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _data != nullptr;
		}

		/// @brief Check whether the instance references a value.
		/// @return @c true if the instance references a value, @c false otherwise.
		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data != nullptr;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &value() noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &value() const noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value and make the instance empty.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &unwrap() noexcept {
			assert(has_value());

			V *data = _data;
			_data = nullptr;
			return *data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get the referenced value.
		/// @pre The instance references a value.
		/// @return The reference to the referenced value.
		PEFF_FORCEINLINE V &operator*() const noexcept {
			assert(has_value());
			return *_data;
		}

		/// @brief Get a pointer to the referenced value.
		/// @pre The instance references a value.
		/// @return The pointer to the referenced value.
		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(has_value());
			return _data;
		}

		/// @brief Get a pointer to the referenced value.
		/// @pre The instance references a value.
		/// @return The pointer to the referenced value.
		PEFF_FORCEINLINE V *operator->() const noexcept {
			assert(has_value());
			return _data;
		}

		/// @brief Construct an empty instance.
		PEFF_FORCEINLINE explicit Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		/// @brief Construct an instance referencing @p data.
		/// @param data The value to be referenced.
		PEFF_FORCEINLINE Option(V &data) noexcept {
			set_value_ref(data);
		}

		/// @brief Construct an empty instance.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		/// @brief Construct an instance by taking the reference of @p rhs from it.
		/// @param rhs The instance to take the reference from.
		PEFF_FORCEINLINE Option(ThisType &&rhs) noexcept {
			PEFF_ASSERT(&rhs != this, "Self-assignment is not allowed!");
			_take_value_from(rhs);
		}

		/// @brief Take the reference of @p rhs from it into this instance.
		/// @note Taking the reference of an instance into itself does nothing.
		/// @param rhs The instance to take the reference from.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			if (this != std::addressof(rhs))
				_take_value_from(rhs);
			return *this;
		}

		/// @brief Set the referenced value.
		/// @param rhs The value to be referenced.
		/// @return The reference to this instance.
		PEFF_FORCEINLINE ThisType &operator=(V &rhs) noexcept {
			set_value_ref(rhs);
			return *this;
		}
	};

	/// @brief The type used for containing an array of optional values or peff::NULLOPT.
	/// @tparam T Type of the optional values.
	/// @tparam length Length of the array.
	template <typename T, size_t length>
	class OptionArray final {
		static_assert(length != 0, "The array must not be empty");

	private:
		alignas(T) char _data[sizeof(T) * length];
		bool _has_value[length] = { false };

		/// @brief Get a pointer to the raw storage of the element at @p index.
		/// @pre @p index is less than @c length.
		/// @param index The index of the element.
		/// @return The pointer to the raw storage.
		[[nodiscard]] PEFF_FORCEINLINE T *_raw_ptr(size_t index) noexcept {
			return reinterpret_cast<T *>(&_data[index * sizeof(T)]);
		}

		/// @brief Get a pointer to the value of the element at @p index.
		/// @pre @p index is less than @c length, and the element contains a value.
		/// @param index The index of the element.
		/// @return The pointer to the value.
		[[nodiscard]] PEFF_FORCEINLINE T *_value_ptr(size_t index) noexcept {
			return std::launder(reinterpret_cast<T *>(&_data[index * sizeof(T)]));
		}

		/// @brief Get a pointer to the value of the element at @p index.
		/// @pre @p index is less than @c length, and the element contains a value.
		/// @param index The index of the element.
		/// @return The pointer to the value.
		[[nodiscard]] PEFF_FORCEINLINE const T *_value_ptr(size_t index) const noexcept {
			return std::launder(reinterpret_cast<const T *>(&_data[index * sizeof(T)]));
		}

		/// @brief Check whether @p data refers to the value of the element at @p index.
		/// @pre @p index is less than @c length.
		/// @param index The index of the element.
		/// @param data The value to be checked.
		/// @return @c true if @p data is the value of the element, @c false otherwise.
		[[nodiscard]] PEFF_FORCEINLINE bool _is_stored(size_t index, const T &data) const noexcept {
			return _has_value[index] &&
				   static_cast<const void *>(std::addressof(data)) ==
					   static_cast<const void *>(&_data[index * sizeof(T)]);
		}

		/// @brief Move the value out of the element at @p index of @p rhs, destroying it in @p rhs.
		/// @pre This array is not @p rhs, and the element at @p index of this array is empty.
		/// @param index The index of the element.
		/// @param rhs The array to take the value from.
		PEFF_FORCEINLINE void _take_value_from(size_t index, OptionArray<T, length> &rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if (rhs._has_value[index]) {
				peff::construct_at<T>(_raw_ptr(index), std::move(*rhs._value_ptr(index)));
				_has_value[index] = true;
				rhs.reset(index);
			}
		}

		/// @brief Move the value out of the element at @p index and destroy it.
		/// @pre The element contains a value.
		/// @param index The index of the element.
		/// @return The extracted value.
		[[nodiscard]] PEFF_FORCEINLINE T _extract(size_t index) noexcept(std::is_nothrow_move_constructible_v<T>) {
			assert(index < length);
			assert(_has_value[index]);

			T data(std::move(*_value_ptr(index)));
			reset(index);
			return data;
		}

	public:
		using value_type = T;

		/// @brief Destroy the value of the element at @p index if there is one, making it empty.
		/// @param index The index of the element.
		PEFF_FORCEINLINE void reset(size_t index) noexcept {
			assert(index < length);

			if (_has_value[index]) {
				if constexpr (std::is_destructible_v<T>)
					peff::destroy_at<T>(_value_ptr(index));
				_has_value[index] = false;
			}
		}

		/// @brief Set the value of the element at @p index by moving @p data into it.
		/// @note Setting the value of an element from itself does nothing.
		/// @param index The index of the element.
		/// @param data The value to be stored.
		PEFF_FORCEINLINE void set_value(size_t index, T &&data) noexcept(std::is_nothrow_move_constructible_v<T>) {
			assert(index < length);

			if (_is_stored(index, data))
				return;

			reset(index);
			peff::construct_at<T>(_raw_ptr(index), std::move(data));
			_has_value[index] = true;
		}

		/// @brief Set the value of the element at @p index by copying @p data into it.
		/// @note Setting the value of an element from itself does nothing.
		/// @param index The index of the element.
		/// @param data The value to be stored.
		PEFF_FORCEINLINE void set_value(size_t index, const T &data) noexcept(std::is_nothrow_copy_constructible_v<T>) {
			assert(index < length);

			if (_is_stored(index, data))
				return;

			reset(index);
			peff::construct_at<T>(_raw_ptr(index), data);
			_has_value[index] = true;
		}

		/// @brief Make the element at @p index empty.
		/// @param index The index of the element.
		/// @param value The empty value, which is always peff::NULLOPT.
		PEFF_FORCEINLINE void set_value(size_t index, NullOption) noexcept {
			reset(index);
		}

		/// @brief Check whether the element at @p index contains a value.
		/// @param index The index of the element.
		/// @return @c true if the element contains a value, @c false otherwise.
		PEFF_FORCEINLINE bool has_value(size_t index) const noexcept {
			assert(index < length);
			return _has_value[index];
		}

		/// @brief Get a reference to the value of the element at @p index.
		/// @pre The element contains a value.
		/// @param index The index of the element.
		/// @return The reference to the value.
		PEFF_FORCEINLINE T &value(size_t index) & noexcept {
			assert(index < length);
			assert(_has_value[index]);
			return *_value_ptr(index);
		}

		/// @brief Get a reference to the value of the element at @p index.
		/// @pre The element contains a value.
		/// @param index The index of the element.
		/// @return The reference to the value.
		PEFF_FORCEINLINE const T &value(size_t index) const & noexcept {
			assert(index < length);
			assert(_has_value[index]);
			return *_value_ptr(index);
		}

		/// @brief Move the value out of the element at @p index.
		/// @pre The element contains a value.
		/// @param index The index of the element.
		/// @return The moved value, while the element becomes empty.
		PEFF_FORCEINLINE T value(size_t index) && noexcept(std::is_nothrow_move_constructible_v<T>) {
			return _extract(index);
		}

		/// @brief Move the value out of the element at @p index.
		/// @pre The element contains a value.
		/// @param index The index of the element.
		/// @return The moved value, while the element becomes empty.
		PEFF_FORCEINLINE T move(size_t index) noexcept(std::is_nothrow_move_constructible_v<T>) {
			return _extract(index);
		}

		/// @brief Construct an empty array.
		PEFF_FORCEINLINE OptionArray() noexcept {
		}

		/// @brief Destroy the value of every element if there is one.
		PEFF_FORCEINLINE ~OptionArray() {
			for (size_t i = 0; i < length; ++i)
				reset(i);
		}

		/// @brief Construct an array by moving the values of @p rhs out of it.
		/// @param rhs The array to take the values from.
		PEFF_FORCEINLINE OptionArray(OptionArray<T, length> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			for (size_t i = 0; i < length; ++i)
				_take_value_from(i, rhs);
		}

		/// @brief Move the values of @p rhs out of it into this array.
		/// @note Moving an array into itself does nothing.
		/// @param rhs The array to take the values from.
		/// @return The reference to this array.
		PEFF_FORCEINLINE OptionArray<T, length> &operator=(OptionArray<T, length> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
			if (this != std::addressof(rhs)) {
				for (size_t i = 0; i < length; ++i) {
					reset(i);
					_take_value_from(i, rhs);
				}
			}
			return *this;
		}
	};
}

#endif
