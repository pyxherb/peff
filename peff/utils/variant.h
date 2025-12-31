#ifndef _PEFF_UTILS_VARIANT_H_
#define _PEFF_UTILS_VARIANT_H_

#include "basedefs.h"
#include <peff/utils/misc.h>
#include <peff/base/alloc.h>

namespace peff {
	namespace details {
		template <typename... Ts>
		struct VariantStorage;

		template <>
		struct VariantStorage<> {};

		template <typename T, typename... Ts>
		struct VariantStorage<T, Ts...> {
			union {
				T value;
				VariantStorage<Ts...> next;
			};
		};

		template <size_t i, typename... Ts>
		struct VariantIndexImpl;

		template <typename T, typename... Ts>
		struct VariantIndexImpl<0, T, Ts...> {
			constexpr static auto value = VariantIndexImpl<index + 1, T, Ts...>::value;
		};

		template <size_t i, typename T, typename... Ts>
		struct VariantIndexImpl<i, T, Ts...> {
			constexpr static auto value = i;
		};

		template <typename T, typename... Ts>
		struct VariantIndex {
			constexpr static auto value = VariantIndexImpl<0, T, Ts...>::value;
		};

		template <size_t index, typename T, typename... Ts>
		PEFF_FORCEINLINE constexpr auto &atVariantStorage(VariantStorage<T, Ts...> &storage) noexcept {
			if constexpr (index == 0) {
				return storage.value;
			} else {
				return atVariantStorage<index - 1, Ts...>(storage.next);
			}
		}

		template <typename T, typename T1, typename... Ts>
		PEFF_FORCEINLINE constexpr void constructAtVariantStorage(VariantStorage<T1, Ts...> &storage, T &&data) {
			if constexpr (std::is_same_v<std::remove_cvref_t<T>>) {
				constructAt<T>(&storage.value, std::forward<T>(value));
			} else {
				constructAtVariantStorage<T, Ts...>(storage.next, std::forward<T>(data));
			}
		}

		template <size_t index, typename T, typename... Ts>
		PEFF_FORCEINLINE constexpr void destroyAtVariantStorage(VariantStorage<T, Ts...> &storage) {
			if constexpr (index == 0) {
				std::destroy_at<T>(&storage.value);
			} else {
				constructAtVariantStorage<index - 1, Ts...>(storage.next, std::forward<T>(data));
			}
		}
	}

	template <typename... Ts>
	class Variant final {
	private:
		details::VariantStorage<Ts...> _storage;
		AutoSizeUInteger<sizeof...(Ts)> _index;

		using Destructor = void (*)(void *) noexcept;

		template <typename T>
		static void _destroyElement(void *ptr) noexcept {
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			details::destroyAtVariantStorage<index>(*static_cast<details::VariantStorage<Ts...> *>(ptr));
		}

		constexpr static Destructor _destructorFnTable[] = { &_destroyElement<Ts>... };

		PEFF_FORCEINLINE constexpr static void _destroy() noexcept {
			_destructorFnTable[_index](&_storage);
		}

	public:
		template <typename T>
		PEFF_FORCEINLINE constexpr Variant(T &&data) noexcept {
			static_assert(IsOneOf<T, Ts...>::value, "Type of the value is not one of the element types");
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			details::constructAtVariantStorage(_storage, std::forward<T>(data));
			_index = index;
		}

		PEFF_FORCEINLINE constexpr ~Variant() {
			_destroy();
		}

		template <size_t index>
		PEFF_FORCEINLINE constexpr auto &getUnchecked() noexcept {
			return details::atVariantStorage<index>(_storage);
		}

		template <size_t index>
		PEFF_FORCEINLINE constexpr const auto &getUnchecked() const noexcept {
			return details::atVariantStorage<index>(_storage);
		}

		template <size_t index>
		PEFF_FORCEINLINE constexpr const auto &get() const noexcept {
			if (index != _index)
				std::terminate();
			return getUnchecked<index>();
		}

		template <size_t index>
		PEFF_FORCEINLINE constexpr auto &get() noexcept {
			if (index != _index)
				std::terminate();
			return getUnchecked<index>();
		}

		template <typename T>
		PEFF_FORCEINLINE constexpr auto &getUnchecked() noexcept {
			static_assert(IsOneOf<T, Ts...>::value, "Type of the value is not one of the element types");
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			return getUnchecked<index>();
		}

		template <typename T>
		PEFF_FORCEINLINE constexpr const auto &getUnchecked() const noexcept {
			static_assert(IsOneOf<T, Ts...>::value, "Type of the value is not one of the element types");
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			return getUnchecked<index>();
		}

		template <typename T>
		PEFF_FORCEINLINE constexpr auto &get() noexcept {
			static_assert(IsOneOf<T, Ts...>::value, "Type of the value is not one of the element types");
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			return get<index>();
		}

		template <typename T>
		PEFF_FORCEINLINE constexpr const auto &get() const noexcept {
			static_assert(IsOneOf<T, Ts...>::value, "Type of the value is not one of the element types");
			constexpr size_t index = details::VariantIndex<std::remove_cvref_t<T>, Ts...>;
			return get<index>();
		}

		PEFF_FORCEINLINE size_t index() const noexcept {
			return _index;
		}
	};
}

#endif
