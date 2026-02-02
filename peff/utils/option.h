#ifndef _PEFF_UTILS_OPTION_H_
#define _PEFF_UTILS_OPTION_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <type_traits>

namespace peff {
	struct NullOption {
	};

	constexpr static NullOption NULL_OPTION;

	template <typename T>
	class Option final {
	private:
		alignas(T) char _data[sizeof(T)];
		bool _hasValue = false;

	public:
		using value_type = T;

		static_assert(std::is_move_constructible_v<T>, "The type must be move constructible");

		PEFF_FORCEINLINE void reset() noexcept {
			if (_hasValue) {
				if (std::is_destructible_v<T>) {
					peff::destroyAt<T>((T *)_data);
				}
			}
			_hasValue = false;
		}

		PEFF_FORCEINLINE void setValue(T &&data) noexcept {
			reset();
			peff::constructAt<T>(((T *)_data), std::move(data));
			_hasValue = true;
		}

		PEFF_FORCEINLINE void setValue(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool hasValue() const noexcept {
			return _hasValue;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _hasValue;
		}

		PEFF_FORCEINLINE T &value() noexcept {
			assert(hasValue());
			return *((T *)_data);
		}

		PEFF_FORCEINLINE const T &value() const noexcept {
			assert(hasValue());
			return *((const T *)_data);
		}

		PEFF_FORCEINLINE T move() noexcept {
			assert(hasValue());
			_hasValue = false;
			return std::move(*((T *)_data));
		}

		PEFF_FORCEINLINE T &operator*() noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE const T &operator*() const noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE std::remove_reference_t<T> *operator->() noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE std::remove_reference_t<const T> *operator->() const noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(T &&data) noexcept {
			setValue(std::move(data));
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.hasValue()) {
				setValue(std::move(*((T *)rhs._data)));
				rhs._hasValue = false;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.hasValue()) {
				setValue(std::move(*((T *)rhs._data)));
				rhs._hasValue = false;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(T &&rhs) noexcept {
			reset();

			setValue(std::move(rhs));
			return *this;
		}
	};

	template <typename T>
	class Option<T &> final {
	private:
		using V = std::remove_reference_t<T>;
		V *_data = nullptr;

	public:
		using value_type = T;

		PEFF_FORCEINLINE void reset() noexcept {
			_data = nullptr;
		}

		PEFF_FORCEINLINE void setValueRef(V &data) noexcept {
			reset();
			_data = &data;
		}

		PEFF_FORCEINLINE void setValue(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool hasValue() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE V &value() noexcept {
			assert(hasValue());
			return *_data;
		}

		PEFF_FORCEINLINE const V &value() const noexcept {
			assert(hasValue());
			return *((const V *)_data);
		}

		PEFF_FORCEINLINE V move() noexcept {
			assert(hasValue());
			V v = std::move(*_data);
			_data = nullptr;
			return v;
		}

		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE const V &operator*() const noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE const V *operator->() const noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(V &data) noexcept {
			setValueRef(data);
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(V &rhs) noexcept {
			reset();
			setValueRef(rhs);
			return *this;
		}
	};

	template <typename T>
	class Option<const T &> final {
	private:
		using V = std::remove_reference_t<T>;
		const V *_data = nullptr;

	public:
		using value_type = T;

		PEFF_FORCEINLINE void reset() noexcept {
			_data = nullptr;
		}

		PEFF_FORCEINLINE void setValueRef(const V &data) noexcept {
			reset();
			_data = &data;
		}

		PEFF_FORCEINLINE void setValue(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool hasValue() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE V &value() noexcept {
			assert(hasValue());
			return *_data;
		}

		PEFF_FORCEINLINE const V &value() const noexcept {
			assert(hasValue());
			return *((const V *)_data);
		}

		PEFF_FORCEINLINE V move() noexcept {
			assert(hasValue());
			V v = std::move(*_data);
			_data = nullptr;
			return v;
		}

		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE const V &operator*() const noexcept {
			assert(hasValue());
			return value();
		}

		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE const V *operator->() const noexcept {
			assert(hasValue());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(const V &data) noexcept {
			setValueRef(data);
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(const V &rhs) noexcept {
			reset();
			setValueRef(rhs);
			return *this;
		}
	};

	template <typename T, size_t length>
	class OptionArray final {
	private:
		alignas(T) char _data[sizeof(T) * length];
		bool _hasValue[length] = { false };

	public:
		using value_type = T;

		PEFF_FORCEINLINE void reset(size_t index) noexcept {
			if (_hasValue[index]) {
				if (std::is_destructible_v<T>) {
					peff::destroyAt<T>((T *)(&_data[index * sizeof(T)]));
				}
			}
			_hasValue[index] = false;
		}

		PEFF_FORCEINLINE void setValue(size_t index, T &&data) noexcept {
			assert(index < length);
			reset(index);
			peff::constructAt<T>(((T *)(&_data[index * sizeof(T)])), std::move(data));
			_hasValue[index] = true;
		}

		PEFF_FORCEINLINE void setValue(size_t index, NullOption) noexcept {
			assert(index < length);
			reset(index);
		}

		PEFF_FORCEINLINE bool hasValue(size_t index) const noexcept {
			assert(index < length);
			return _hasValue[index];
		}

		PEFF_FORCEINLINE T &value(size_t index) noexcept {
			assert(index < length);
			assert(hasValue(index));
			return *((T *)(&_data[index * sizeof(T)]));
		}

		PEFF_FORCEINLINE const T &value(size_t index) const noexcept {
			assert(index < length);
			assert(hasValue(index));
			return *((const T *)(&_data[index * sizeof(T)]));
		}

		PEFF_FORCEINLINE T move(size_t index) noexcept {
			assert(index < length);
			assert(hasValue(index));
			_hasValue[index] = false;
			return std::move(*((T *)(&_data[index * sizeof(T)])));
		}

		PEFF_FORCEINLINE OptionArray() noexcept {
		}

		PEFF_FORCEINLINE ~OptionArray() {
			for (size_t i = 0; i < length; ++i)
				reset(i);
		}

		PEFF_FORCEINLINE OptionArray(OptionArray<T, length> &&rhs) noexcept {
			for (size_t i = 0; i < length; ++i) {
				if (rhs.hasValue(i)) {
					setValue(i, std::move(*((T *)&rhs._data[i * sizeof(T)])));
					rhs._hasValue[i] = false;
				}
			}
		}

		PEFF_FORCEINLINE OptionArray<T, length> &operator=(OptionArray<T, length> &&rhs) noexcept {
			if (&rhs == this)
				return *this;
			for (size_t i = 0; i < length; ++i) {
				reset(i);
				if (rhs.hasValue(i)) {
					setValue(i, std::move(*((T *)&rhs._data[i * sizeof(T)])));
					rhs._hasValue[i] = false;
				}
			}
			return *this;
		}
	};
}

#endif
