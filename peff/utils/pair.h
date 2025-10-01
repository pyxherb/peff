#ifndef _PEFF_UTILS_PAIR_H_
#define _PEFF_UTILS_PAIR_H_

#include "basedefs.h"
#include <type_traits>

namespace peff {
	template<typename T, bool IsKey, bool IsEmpty>
	class CompressedElement;

	template <typename T, bool IsKey>
	class CompressedElement<T, IsKey, false> {
	private:
		T _value;

	public:
		PEFF_FORCEINLINE CompressedElement(T &&value) : _value(std::move(value)) {
		}

		PEFF_FORCEINLINE T &value() noexcept { return _value; }
		PEFF_FORCEINLINE const T &value() const noexcept { return _value; }
	};

	template <typename T, bool IsKey>
	class CompressedElement<T, IsKey, true> : public T {
	public:
		PEFF_FORCEINLINE CompressedElement(T &&value) : T(std::move(value)) {
		}

		PEFF_FORCEINLINE T &value() noexcept { return *static_cast<T *>(this); }
		PEFF_FORCEINLINE const T &value() const noexcept {
			return *static_cast<const T *>(this);
		}
	};

	template <typename K, typename V>
	class CompressedPair : private CompressedElement<K, true, std::is_empty_v<K>>,
						   private CompressedElement<V, false, std::is_empty_v<V>> {
	private:
		using First = CompressedElement<K, true, std::is_empty_v<K>>;
		using Second = CompressedElement<V, false, std::is_empty_v<V>>;

	public:
		template <typename K2, typename V2>
		PEFF_FORCEINLINE constexpr CompressedPair(K2 &&k, V2 &&v) : First::CompressedElement(std::forward<K2>(k)), Second::CompressedElement(std::forward<V2>(v)) {}

		PEFF_FORCEINLINE K &first() noexcept {
			return static_cast<First &>(*this).value();
		}
		PEFF_FORCEINLINE const K &first() const noexcept {
			return static_cast<const First &>(*this).value();
		}
		PEFF_FORCEINLINE V &second() noexcept {
			return static_cast<Second &>(*this).value();
		}
		PEFF_FORCEINLINE const V &second() const noexcept {
			return static_cast<const Second &>(*this).value();
		}
	};
}

#endif
