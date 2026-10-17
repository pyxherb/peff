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
			peff::construct_at<T>((T *)_buf, std::move(data));
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
			return get();
		}
		PEFF_FORCEINLINE const T &operator*() const {
			return get();
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
		PEFF_FORCEINLINE void move_from(T &&src) {
			peff::construct_at((T *)_buf, std::move(src));
		}
		PEFF_FORCEINLINE Uninit<T> &operator=(T &&rhs) noexcept {
			peff::construct_at((T *)_buf, std::move(rhs));
			return *this;
		}
	};
	template <class T>
	struct TypeIdentity {
		using type = T;
	};

	template <class T>
	using TypeIdentityType = typename peff::TypeIdentity<T>::type;

	enum class ControlFlow : bool {
		Break = false,
		Continue = true
	};

	enum class Endian : uint8_t {
		Little = 0,
		Big
	};

	template <typename T>
	PEFF_FORCEINLINE void move_assign_or_move_construct(T &lhs, T &&rhs) noexcept {
		if constexpr (std::is_move_assignable_v<T>) {
			lhs = std::move(rhs);
		} else {
			static_assert(std::is_move_constructible_v<T>, "The type must at least be move-constructible");
			std::destroy_at<T>(&lhs);
			peff::construct_at<T>(&lhs, std::move(rhs));
		}
	}

	PEFF_BASE_API Endian test_native_endian();

	template <typename T>
	PEFF_FORCEINLINE constexpr T ceil_align_to(T data, T alignment) {
		static_assert(std::is_unsigned_v<T>, "ceil_align_to only applies to unsigned types");
		kd_assert(alignment);
		if ((alignment & (alignment - 1)) == 0)
			return (data + (alignment - 1)) & ~(alignment - 1);
		return data + ((alignment - data % alignment) % alignment);
	}

	template <typename T, T alignment>
	PEFF_FORCEINLINE constexpr T ceil_align_to(T data) {
		static_assert(std::is_unsigned_v<T>, "ceil_align_to only applies to unsigned types");
		static_assert(alignment != 0, "alignment must not be 0");
		if constexpr ((alignment & (alignment - 1)) == 0) {
			return (data + (alignment - 1)) & ~(alignment - 1);
		}
		return data + ((alignment - data % alignment) % alignment);
	}

	template <typename T>
	PEFF_FORCEINLINE constexpr size_t ceil_align_to_type(size_t data) {
		return (data + (alignof(T) - 1)) & ~(alignof(T) - 1);
	}

	template <typename T>
	PEFF_FORCEINLINE constexpr T floor_align_to(T data, T alignment) {
		static_assert(std::is_unsigned_v<T>, "floor_align_to only applies to unsigned types");
		kd_assert(alignment);
		return data - data % alignment;
	}

	template <typename T, T alignment>
	PEFF_FORCEINLINE constexpr T floor_align_to(T data) {
		static_assert(std::is_unsigned_v<T>, "floor_align_to only applies to unsigned types");
		static_assert(alignment != 0, "alignment must not be 0");
		if constexpr ((alignment & (alignment - 1)) == 0) {
			return data & ~(alignment - 1);
		}
		return data - data % alignment;
	}

	template <typename T>
	PEFF_FORCEINLINE constexpr size_t floor_align_to_type(size_t data) {
		return data & ~(alignof(T) - 1);
	}
}

#endif
