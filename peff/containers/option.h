#ifndef _PEFF_CONTAINERS_OPTION_H_
#define _PEFF_CONTAINERS_OPTION_H_

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
			_hasValue = true;
		}

		PEFF_FORCEINLINE void setValue(T &&data) noexcept {
			reset();
			*((T *)_data) = std::move(data);
			_hasValue = true;
		}

		PEFF_FORCEINLINE void setValue(NullOption) noexcept {
			reset();
		}

		PEFF_FORCEINLINE bool hasValue() const noexcept {
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

		PEFF_FORCEINLINE T &operator*() noexcept {
			assert(hasValue());
			return *((T *)_data);
		}

		PEFF_FORCEINLINE const T &operator*() const noexcept {
			assert(hasValue());
			return *((const T *)_data);
		}

		PEFF_FORCEINLINE T *operator->() noexcept {
			assert(hasValue());
			return ((T *)_data);
		}

		PEFF_FORCEINLINE const T *operator->() const noexcept {
			assert(hasValue());
			return ((const T *)_data);
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
			}
		}

		PEFF_FORCEINLINE Option<T> &operator=(Option<T>&& rhs) noexcept {
			reset();

			if (rhs.hasValue()) {
				setValue(std::move(*((T *)rhs._data)));
			}
			return *this;
		}
	};
}

#endif
