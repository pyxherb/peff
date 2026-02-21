#ifndef _PEFF_ETILS_RESELT_H_
#define _PEFF_ETILS_RESELT_H_

#include "basedefs.h"
#include "misc.h"
#include <peff/base/alloc.h>
#include <type_traits>

namespace peff {
	struct NullResult {
	};

	constexpr static NullResult NELL_RESELT;

	enum class ResultIndex : uint8_t {
		_Moved = 0,
		Value,
		Error
	};

	template <typename T, typename E>
	class Result final {
	private:
		alignas((std::max)(alignof(T), alignof(E))) char _data[(std::max)(sizeof(T), sizeof(E))];
		ResultIndex _index = ResultIndex::_Moved;

	public:
		using value_type = T;
		using error_type = E;

		static_assert(!std::is_same_v<T, E>, "The value and the error type must be different");
		static_assert(std::is_move_constructible_v<T>, "The value type must be move constructible");
		static_assert(std::is_move_constructible_v<E>, "The error type must be move constructible");

		PEFF_FORCEINLINE void reset() noexcept {
			switch (_index) {
				case ResultIndex::_Moved:
					break;
				case ResultIndex::Value:
					if (std::is_destructible_v<T>) {
						peff::destroyAt<T>((T *)_data);
					}
					_index = ResultIndex::_Moved;
					break;
				case ResultIndex::Error:
					if (std::is_destructible_v<E>) {
						peff::destroyAt<E>((E *)_data);
					}
					_index = ResultIndex::_Moved;
					break;
				default:
					PEFF_UNREACHABLE();
			}
		}

		PEFF_FORCEINLINE void setValue(T &&data) noexcept {
			reset();
			peff::constructAt<T>(((T *)_data), std::move(data));
			_index = ResultIndex::Value;
		}

		PEFF_FORCEINLINE void setError(E &&data) noexcept {
			reset();
			peff::constructAt<E>(((E *)_data), std::move(data));
			_index = ResultIndex::Error;
		}

		PEFF_FORCEINLINE void setNone(NullResult) noexcept {
			assert(_index != ResultIndex::_Moved);
			reset();
		}

		PEFF_FORCEINLINE ResultIndex index() const noexcept {
			assert(_index != ResultIndex::_Moved);
			return _index;
		}

		PEFF_FORCEINLINE bool hasValue() const noexcept {
			assert(_index != ResultIndex::_Moved);
			return _index == ResultIndex::Value;
		}

		PEFF_FORCEINLINE bool hasError() const noexcept {
			assert(_index != ResultIndex::_Moved);
			return _index == ResultIndex::Error;
		}

		PEFF_FORCEINLINE explicit operator bool() const noexcept {
			assert(_index != ResultIndex::_Moved);
			return hasValue();
		}

		PEFF_FORCEINLINE T &value() & noexcept {
			assert(hasValue());
			return *((T *)_data);
		}

		PEFF_FORCEINLINE const T &value() const & noexcept {
			assert(hasValue());
			return *((const T *)_data);
		}

		PEFF_FORCEINLINE T value() && noexcept {
			assert(hasValue());
			return std::move(*((T *)_data));
		}

		PEFF_FORCEINLINE E &error() & noexcept {
			assert(hasError());
			return *((E *)_data);
		}

		PEFF_FORCEINLINE const E &error() const & noexcept {
			assert(hasError());
			return *((const E *)_data);
		}

		PEFF_FORCEINLINE E error() && noexcept {
			assert(hasError());
			return std::move(*((E *)_data));
		}

		Result() = delete;

		PEFF_FORCEINLINE ~Result() {
			reset();
		}

		PEFF_FORCEINLINE Result(T &&data) noexcept {
			setValue(std::move(data));
		}

		PEFF_FORCEINLINE Result(E &&data) noexcept {
			setError(std::move(data));
		}

		PEFF_FORCEINLINE Result(NullResult) noexcept {
		}

		PEFF_FORCEINLINE Result(Result<T, E> &&rhs) noexcept {
			switch (rhs._index) {
				case ResultIndex::_Moved:
					PEFF_UNREACHABLE();
					break;
				case ResultIndex::Value:
					setValue(std::move(*((T *)rhs._data)));
					rhs._index = ResultIndex::_Moved;
					break;
				case ResultIndex::Error:
					setError(std::move(*((E *)rhs._data)));
					rhs._index = ResultIndex::_Moved;
					break;
				default:
					PEFF_UNREACHABLE();
			}
		}

		PEFF_FORCEINLINE Result<T, E> &operator=(Result<T, E> &&rhs) noexcept {
			reset();

			switch (rhs._index) {
				case ResultIndex::_Moved:
					PEFF_UNREACHABLE();
					break;
				case ResultIndex::Value:
					setValue(std::move(*((T *)rhs._data)));
					rhs._index = ResultIndex::_Moved;
					break;
				case ResultIndex::Error:
					setError(std::move(*((E *)rhs._data)));
					rhs._index = ResultIndex::_Moved;
					break;
				default:
					PEFF_UNREACHABLE();
			}

			return *this;
		}

		PEFF_FORCEINLINE Result<T, E> &operator=(T &&rhs) noexcept {
			reset();

			setValue(std::move(rhs));
			return *this;
		}

		PEFF_FORCEINLINE Result<T, E> &operator=(E &&rhs) noexcept {
			reset();

			setError(std::move(rhs));
			return *this;
		}
	};
}

#endif
