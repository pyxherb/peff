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
	};

	template <typename T>
	class Option<T &> final {
	private:
		T *_data;
		bool _hasValue = false;

	public:
		using value_type = T &;

		PEFF_FORCEINLINE void reset() noexcept {
			_hasValue = false;
		}

		PEFF_FORCEINLINE void setValueRef(T &data) noexcept {
			reset();
			_data = &data;
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
			return *_data;
		}

		PEFF_FORCEINLINE const T &value() const noexcept {
			assert(hasValue());
			return *((const T *)_data);
		}

		PEFF_FORCEINLINE T move() noexcept {
			assert(hasValue());
			_hasValue = false;
			return std::move(*_data);
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

		PEFF_FORCEINLINE Option(T &data) noexcept {
			setValueRef(data);
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._hasValue = false;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.hasValue()) {
				setValueRef(**((std::remove_reference_t<T> **)rhs._data));
				rhs._hasValue = false;
			}
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
