#ifndef _PEFF_BASE_MISC_H_
#define _PEFF_BASE_MISC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <type_traits>
#include <cstring>

namespace peff {
	template <typename T>
	class Uninit final {
	private:
		alignas(T) char _buf[sizeof(T)];

	public:
		Uninit() noexcept = default;
		PEFF_FORCEINLINE explicit Uninit(T &&data) noexcept {
			peff::constructAt<T>((T *)_buf, std::move(data));
		}
		Uninit(const Uninit<T> &) = delete;
		PEFF_FORCEINLINE ~Uninit() {
		}
		PEFF_FORCEINLINE T &get() {
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE const T &get() const {
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE T &operator*() {
			return *(T *)get();
		}
		PEFF_FORCEINLINE const T &operator*() const {
			return *(T *)get();
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
		PEFF_FORCEINLINE void destroy() {
			return std::destroy_at<T>((T *)_buf);
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
		PEFF_FORCEINLINE Uninit<T> &operator=(T &&rhs) noexcept {
			constructAt((T *)_buf, std::move(rhs));
			return *this;
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

	template <typename T>
	PEFF_FORCEINLINE void moveAssignOrMoveConstruct(T &lhs, T &&rhs) noexcept {
		if constexpr (std::is_move_assignable_v<T>) {
			lhs = std::move(rhs);
		} else {
			static_assert(std::is_move_constructible_v<T>, "The type must at least be move-constructible");
			std::destroy_at<T>(&lhs);
			peff::constructAt<T>(&lhs, std::move(lhs));
		}
	}

	PEFF_BASE_API Endian testNativeEndian();
}

#endif
