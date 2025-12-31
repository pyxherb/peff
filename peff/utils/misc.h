#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include "basedefs.h"
#include <type_traits>
#include <functional>

namespace peff {
	template <uint64_t Max>
	using AutoSizeUInteger =
		std::conditional_t<
			Max <= UINT8_MAX,
			uint8_t,
			std::conditional_t<
				Max <= UINT16_MAX,
				uint16_t,
				std::conditional_t<
					Max <= UINT32_MAX,
					uint32_t,
					uint64_t>>>;
	/*
	template <typename T>
	class MinSizeOf {
		constexpr static size_t value = sizeof(T);
	};

	template <typename T, typename... Ts>
	class MinSizeOf {
	private:
		constexpr static size_t _szRest = MaxSizeOf<Ts...>::value;

	public:
		constexpr static size_t value = sizeof(T) < _szRest ? sizeof(T) : _szRest;
	};

	template <typename T>
	class MaxSizeOf {
		constexpr static size_t value = sizeof(T);
	};

	template <typename T, typename... Ts>
	class MaxSizeOf {
	private:
		constexpr static size_t _szRest = MaxSizeOf<Ts...>::value;

	public:
		constexpr static size_t value = sizeof(T) > _szRest ? sizeof(T) : _szRest;
	};*/

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
}

#endif
