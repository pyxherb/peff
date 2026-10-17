#ifndef _PEFF_CONTAINERS_MISC_H_
#define _PEFF_CONTAINERS_MISC_H_

#include "basedefs.h"
#include <cstdint>
#include <type_traits>
#include <utility>

#if PEFF_CPLUSPLUS >= 202002L
#include <concepts>
#endif

namespace peff {
	enum class IteratorDirection : bool {
		Forward = 0,
		Reversed,
	};

	/// @brief Check whether a type declares a static iterator direction.
	/// @tparam T Type to be checked.
	template <typename T, typename = void>
	struct HasStaticIteratorDirection : std::false_type {
	};

	template <typename T>
	struct HasStaticIteratorDirection<T, std::void_t<decltype(T::ITERATOR_DIRECTION)>>
		: std::bool_constant<std::is_same_v<std::remove_cv_t<decltype(T::ITERATOR_DIRECTION)>, IteratorDirection>> {
	};

	/// @brief Check whether a type exposes a runtime direction data member.
	/// @tparam T Type to be checked.
	template <typename T, typename = void>
	struct HasIteratorDirectionField : std::false_type {
	};

	template <typename T>
	struct HasIteratorDirectionField<T, std::void_t<decltype(std::declval<const T &>().direction)>>
		: std::bool_constant<std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<const T &>().direction)>>, IteratorDirection>> {
	};

	/// @brief Check whether a type exposes a runtime direction accessor.
	/// @tparam T Type to be checked.
	template <typename T, typename = void>
	struct HasIteratorDirectionMethod : std::false_type {
	};

	template <typename T>
	struct HasIteratorDirectionMethod<T, std::void_t<decltype(std::declval<const T &>().direction())>>
		: std::bool_constant<std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<const T &>().direction())>>, IteratorDirection>> {
	};

	/// @brief Check whether a type exposes its iterator direction, statically or at runtime.
	/// @tparam T Type to be checked.
	template <typename T>
	struct HasIteratorDirection
		: std::bool_constant<
			  HasStaticIteratorDirection<T>::value ||
			  HasIteratorDirectionField<T>::value ||
			  HasIteratorDirectionMethod<T>::value> {
	};

	/// @brief Check whether a type exposes its iterator direction, statically or at runtime.
	/// @tparam T Type to be checked.
	template <typename T>
	inline constexpr bool has_iterator_direction_v = HasIteratorDirection<T>::value;

	/// @brief Get the direction of an iterator.
	/// @tparam T Iterator type.
	/// @param iterator Iterator whose direction is queried.
	/// @return Direction of the iterator.
	template <typename T>
	PEFF_FORCEINLINE constexpr IteratorDirection iterator_direction(const T &iterator) noexcept {
		static_assert(has_iterator_direction_v<T>, "The type does not expose an iterator direction");

		if constexpr (HasStaticIteratorDirection<T>::value)
			return T::ITERATOR_DIRECTION;
		else if constexpr (HasIteratorDirectionField<T>::value)
			return iterator.direction;
		else
			return iterator.direction();
	}

#if PEFF_CPLUSPLUS >= 202002L
	/// @brief Check whether a type satisfies the common iterator protocol.
	/// @tparam T Type to be checked.
	template <typename T>
	concept IteratorTrait = has_iterator_direction_v<T> && requires(const T &const_iterator, T &iterator) {
		{ const_iterator.next() } -> std::same_as<T>;
		{ iterator++ } -> std::same_as<T>;
		{ ++iterator } -> std::same_as<T &>;
		*iterator;
	};

	/// @brief Check whether a type satisfies the bidirectional iterator protocol.
	/// @tparam T Type to be checked.
	template <typename T>
	concept BidirIteratorTrait = IteratorTrait<T> && requires(const T &const_iterator, T &iterator) {
		{ const_iterator.prev() } -> std::same_as<T>;
		{ iterator-- } -> std::same_as<T>;
		{ --iterator } -> std::same_as<T &>;
	};
#endif

	/// @brief Check whether a type satisfies the common iterator protocol.
	/// @tparam T Type to be checked.
#if PEFF_CPLUSPLUS >= 202002L
	template <typename T, typename = void>
	struct IsIterator : std::bool_constant<IteratorTrait<T>> {
	};
#else
	template <typename T, typename = void>
	struct IsIterator : std::false_type {
	};

	template <typename T>
	struct IsIterator<
		T,
		std::void_t<
			std::enable_if_t<has_iterator_direction_v<T>>,
			decltype(std::declval<const T &>().next()),
			decltype(std::declval<T &>()++),
			decltype(++std::declval<T &>()),
			decltype(*std::declval<T &>())>>
		: std::bool_constant<
			  std::is_same_v<decltype(std::declval<const T &>().next()), T> &&
			  std::is_same_v<decltype(std::declval<T &>()++), T> &&
			  std::is_same_v<decltype(++std::declval<T &>()), T &>> {
	};
#endif

	/// @brief Check whether a type satisfies the bidirectional iterator protocol.
	/// @tparam T Type to be checked.
#if PEFF_CPLUSPLUS >= 202002L
	template <typename T, typename = void>
	struct IsBidirIterator : std::bool_constant<BidirIteratorTrait<T>> {
	};
#else
	template <typename T, typename = void>
	struct IsBidirIterator : std::false_type {
	};

	template <typename T>
	struct IsBidirIterator<
		T,
		std::void_t<
			decltype(std::declval<const T &>().prev()),
			decltype(std::declval<T &>()--),
			decltype(--std::declval<T &>())>>
		: std::bool_constant<
			  IsIterator<T>::value &&
			  std::is_same_v<decltype(std::declval<const T &>().prev()), T> &&
			  std::is_same_v<decltype(std::declval<T &>()--), T> &&
			  std::is_same_v<decltype(--std::declval<T &>()), T &>> {
	};
#endif
}

#endif
