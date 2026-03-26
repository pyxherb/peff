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
		bool _has_value = false;

	public:
		using value_type = T;

		static_assert(std::is_move_constructible_v<T>, "The type must be move constructible");

		PEFF_FORCEINLINE void reset() noexcept {
			if (_has_value) {
				if (std::is_destructible_v<T>) {
					peff::destroy_at<T>((T *)_data);
				}
			}
			_has_value = false;
		}

		PEFF_FORCEINLINE void set_value(T &&data) noexcept {
			reset();
			peff::construct_at<T>(((T *)_data), std::move(data));
			_has_value = true;
		}

		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _has_value;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _has_value;
		}

		PEFF_FORCEINLINE T &value() noexcept {
			assert(has_value());
			return *((T *)_data);
		}

		PEFF_FORCEINLINE const T &value() const noexcept {
			assert(has_value());
			return *((const T *)_data);
		}

		PEFF_FORCEINLINE T move() noexcept {
			assert(has_value());
			_has_value = false;
			return std::move(*((T *)_data));
		}

		PEFF_FORCEINLINE T &operator*() noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE const T &operator*() const noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE std::remove_reference_t<T> *operator->() noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE std::remove_reference_t<const T> *operator->() const noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(T &&data) noexcept {
			set_value(std::move(data));
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.has_value()) {
				set_value(std::move(*((T *)rhs._data)));
				rhs._has_value = false;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.has_value()) {
				set_value(std::move(*((T *)rhs._data)));
				rhs._has_value = false;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(T &&rhs) noexcept {
			reset();

			set_value(std::move(rhs));
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

		PEFF_FORCEINLINE void set_value_ref(V &data) noexcept {
			reset();
			_data = &data;
		}

		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE V &value() noexcept {
			assert(has_value());
			return *_data;
		}

		PEFF_FORCEINLINE const V &value() const noexcept {
			assert(has_value());
			return *((const V *)_data);
		}

		PEFF_FORCEINLINE V move() noexcept {
			assert(has_value());
			V v = std::move(*_data);
			_data = nullptr;
			return v;
		}

		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE const V &operator*() const noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE const V *operator->() const noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(V &data) noexcept {
			set_value_ref(data);
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.has_value()) {
				set_value_ref(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.has_value()) {
				set_value_ref(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(V &rhs) noexcept {
			reset();
			set_value_ref(rhs);
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

		PEFF_FORCEINLINE void set_value_ref(const V &data) noexcept {
			reset();
			_data = &data;
		}

		PEFF_FORCEINLINE void set_value(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool has_value() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _data;
		}

		PEFF_FORCEINLINE V &value() noexcept {
			assert(has_value());
			return *_data;
		}

		PEFF_FORCEINLINE const V &value() const noexcept {
			assert(has_value());
			return *((const V *)_data);
		}

		PEFF_FORCEINLINE V move() noexcept {
			assert(has_value());
			V v = std::move(*_data);
			_data = nullptr;
			return v;
		}

		PEFF_FORCEINLINE V &operator*() noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE const V &operator*() const noexcept {
			assert(has_value());
			return value();
		}

		PEFF_FORCEINLINE V *operator->() noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE const V *operator->() const noexcept {
			assert(has_value());
			return &value();
		}

		PEFF_FORCEINLINE Option() noexcept {
		}

		PEFF_FORCEINLINE ~Option() {
			reset();
		}

		PEFF_FORCEINLINE Option(const V &data) noexcept {
			set_value_ref(data);
		}

		PEFF_FORCEINLINE Option(NullOption) noexcept {
		}

		PEFF_FORCEINLINE Option(Option<T> &&rhs) noexcept {
			if (rhs.has_value()) {
				set_value_ref(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T> &&rhs) noexcept {
			reset();

			if (rhs.has_value()) {
				set_value_ref(**((std::remove_reference_t<T> **)rhs._data));
				rhs._data = nullptr;
			}
			return *this;
		}

		PEFF_FORCEINLINE Option<T> &operator=(const V &rhs) noexcept {
			reset();
			set_value_ref(rhs);
			return *this;
		}
	};

	template <typename T, size_t length>
	class OptionArray final {
	private:
		alignas(T) char _data[sizeof(T) * length];
		bool _has_value[length] = { false };

	public:
		using value_type = T;

		PEFF_FORCEINLINE void reset(size_t index) noexcept {
			if (_has_value[index]) {
				if (std::is_destructible_v<T>) {
					peff::destroy_at<T>((T *)(&_data[index * sizeof(T)]));
				}
			}
			_has_value[index] = false;
		}

		PEFF_FORCEINLINE void set_value(size_t index, T &&data) noexcept {
			assert(index < length);
			reset(index);
			peff::construct_at<T>(((T *)(&_data[index * sizeof(T)])), std::move(data));
			_has_value[index] = true;
		}

		PEFF_FORCEINLINE void set_value(size_t index, NullOption) noexcept {
			assert(index < length);
			reset(index);
		}

		PEFF_FORCEINLINE bool has_value(size_t index) const noexcept {
			assert(index < length);
			return _has_value[index];
		}

		PEFF_FORCEINLINE T &value(size_t index) noexcept {
			assert(index < length);
			assert(has_value(index));
			return *((T *)(&_data[index * sizeof(T)]));
		}

		PEFF_FORCEINLINE const T &value(size_t index) const noexcept {
			assert(index < length);
			assert(has_value(index));
			return *((const T *)(&_data[index * sizeof(T)]));
		}

		PEFF_FORCEINLINE T move(size_t index) noexcept {
			assert(index < length);
			assert(has_value(index));
			_has_value[index] = false;
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
				if (rhs.has_value(i)) {
					set_value(i, std::move(*((T *)&rhs._data[i * sizeof(T)])));
					rhs._has_value[i] = false;
				}
			}
		}

		PEFF_FORCEINLINE OptionArray<T, length> &operator=(OptionArray<T, length> &&rhs) noexcept {
			if (&rhs == this)
				return *this;
			for (size_t i = 0; i < length; ++i) {
				reset(i);
				if (rhs.has_value(i)) {
					set_value(i, std::move(*((T *)&rhs._data[i * sizeof(T)])));
					rhs._has_value[i] = false;
				}
			}
			return *this;
		}
	};
}

#endif
