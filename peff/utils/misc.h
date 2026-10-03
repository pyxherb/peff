#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include "basedefs.h"
#include <type_traits>
#include <compare>
#include <peff/base/assert.h>
#include <cstdint>

namespace peff {
	template <uintmax_t Max>
	struct AutoSizeUInteger {
		using type =
			std::conditional_t<
				Max <= UINT8_MAX,
				uint8_t,
				std::conditional_t<
					Max <= UINT16_MAX,
					uint16_t,
					std::conditional_t<
						Max <= UINT32_MAX,
						uint32_t,
						std::conditional_t<
							Max <= UINT64_MAX,
							uint64_t,
							uintmax_t>>>>;
	};

	template <typename... Ts>
	struct IsOneOf {
		constexpr static bool value = false;
	};

	template <typename T, typename U, typename... Ts>
	struct IsOneOf<T, U, Ts...> {
		constexpr static bool value =
			std::is_same_v<T, U>
				? true
				: IsOneOf<T, Ts...>::value;
	};

	struct PartialOrd {
	public:
		enum class _PartialOrdKind : int8_t {
			Lt = -1,
			Eq = 0,
			Gt = 1,
			Unordered
		};
		_PartialOrdKind _kind;

		PartialOrd() = default;
		PEFF_FORCEINLINE constexpr PartialOrd(_PartialOrdKind kind) : _kind(kind) {}
		constexpr PartialOrd(const PartialOrd &) = default;
		constexpr PartialOrd(PartialOrd &&) = default;
		constexpr PartialOrd &operator=(const PartialOrd &) = default;
		constexpr PartialOrd &operator=(PartialOrd &&) = default;

		/// @brief Check whether this ordering is greater than zero.
		/// @param zero Must be zero, any other value fails the assertion.
		/// @return true if this ordering is greater than zero, false otherwise (including unordered).
		PEFF_FORCEINLINE bool operator>(uint8_t zero) const noexcept {
			PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
			return _kind == _PartialOrdKind::Gt;
		}
		/// @brief Check whether this ordering is less than zero.
		/// @param zero Must be zero, any other value fails the assertion.
		/// @return true if this ordering is less than zero, false otherwise (including unordered).
		PEFF_FORCEINLINE bool operator<(uint8_t zero) const noexcept {
			PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
			return _kind == _PartialOrdKind::Lt;
		}
		/// @brief Check whether this ordering is equal to zero.
		/// @param zero Must be zero, any other value fails the assertion.
		/// @return true if this ordering is equal to zero, false otherwise (including unordered).
		PEFF_FORCEINLINE bool operator==(uint8_t zero) const noexcept {
			PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
			return _kind == _PartialOrdKind::Eq;
		}

#if PEFF_CPLUSPLUS >= 202002L
		PEFF_FORCEINLINE explicit constexpr operator std::partial_ordering() const noexcept {
			switch (_kind) {
				case _PartialOrdKind::Lt:
					return std::partial_ordering::less;
				case _PartialOrdKind::Gt:
					return std::partial_ordering::greater;
				case _PartialOrdKind::Eq:
					return std::partial_ordering::equivalent;
				default:
					return std::partial_ordering::unordered;
			}
		}

		PEFF_FORCEINLINE explicit constexpr PartialOrd(std::partial_ordering ord) noexcept {
			if (ord == std::partial_ordering::less)
				_kind = _PartialOrdKind::Lt;
			else if (ord == std::partial_ordering::greater)
				_kind = _PartialOrdKind::Gt;
			else if (ord == std::partial_ordering::equivalent)
				_kind = _PartialOrdKind::Eq;
			else
				_kind = _PartialOrdKind::Unordered;
		}

		PEFF_FORCEINLINE constexpr PartialOrd &operator=(std::partial_ordering ord) noexcept {
			if (ord == std::partial_ordering::less)
				_kind = _PartialOrdKind::Lt;
			else if (ord == std::partial_ordering::greater)
				_kind = _PartialOrdKind::Gt;
			else if (ord == std::partial_ordering::equivalent)
				_kind = _PartialOrdKind::Eq;
			else
				_kind = _PartialOrdKind::Unordered;
			return *this;
		}
#endif
	};

	constexpr static inline PartialOrd PARTIAL_LT = { PartialOrd::_PartialOrdKind::Lt },
									   PARTIAL_GT = { PartialOrd::_PartialOrdKind::Gt },
									   PARTIAL_EQ = { PartialOrd::_PartialOrdKind::Eq },
									   PARTIAL_UNORDERED = { PartialOrd::_PartialOrdKind::Unordered };

	/// @brief The error type reporting that a three-way comparison failed, that is, the compared
	///        operands are unordered (peff::PARTIAL_UNORDERED). Operations that may encounter such
	///        a comparison report it as the error variant of a peff::Result.
	struct Unordered {
	};
}

PEFF_FORCEINLINE bool operator>(uint8_t zero, peff::PartialOrd ord) noexcept {
	PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
	return ord < zero;
}
PEFF_FORCEINLINE bool operator<(uint8_t zero, peff::PartialOrd ord) noexcept {
	PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
	return ord > zero;
}
PEFF_FORCEINLINE bool operator==(uint8_t zero, peff::PartialOrd ord) noexcept {
	PEFF_ASSERT(zero == 0, "PartialOrd is only comparable with 0");
	return ord == zero;
}

#endif
