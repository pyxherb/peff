#ifndef _PEFF_BASE_MISC_H_
#define _PEFF_BASE_MISC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <type_traits>
#include <cstring>

namespace peff {
	template <typename T>
	class Uninitialized final {
	private:
		alignas(T) char _buf[sizeof(T)];

	public:
		PEFF_FORCEINLINE explicit Uninitialized() {
		}
		Uninitialized(const Uninitialized<T> &) = delete;
		PEFF_FORCEINLINE explicit Uninitialized(Uninitialized<T> &&rhs) {
			memmove(_buf, rhs._buf, sizeof(T));
		}
		PEFF_FORCEINLINE ~Uninitialized() {
		}
		PEFF_FORCEINLINE T &operator*() {
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE const T &operator*() const {
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE T *operator->() {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE const T *operator->() const {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE T &&release() {
			return std::move(*(T *)_buf);
		}
		PEFF_FORCEINLINE T *data() {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE const T *data() const {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE void moveFrom(T &&src) {
			constructAt((T *)_buf, std::move(src));
		}
	};

	enum class ControlFlow : bool {
		Break = false,
		Continue = true
	};

	enum class Endian : uint8_t {
		Little = 0,
		Big
	};

	PEFF_BASE_API Endian testNativeEndian();
}

#endif
