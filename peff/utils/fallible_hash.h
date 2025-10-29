#ifndef _PEFF_UTILS_FALLIBLE_HASH_H_
#define _PEFF_UTILS_FALLIBLE_HASH_H_

#include "basedefs.h"
#include "option.h"

namespace peff {
	template <typename T>
	struct FallibleHasher {
		static_assert(!std::is_same_v<T, T>, "Hasher was not found");
	};

	template <>
	struct FallibleHasher<signed char> {
		PEFF_FORCEINLINE Option<size_t> operator()(signed char x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<unsigned char> {
		PEFF_FORCEINLINE Option<size_t> operator()(unsigned char x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<char> {
		PEFF_FORCEINLINE Option<size_t> operator()(char x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<short> {
		PEFF_FORCEINLINE Option<size_t> operator()(short x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<unsigned short> {
		PEFF_FORCEINLINE Option<size_t> operator()(unsigned short x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<int> {
		PEFF_FORCEINLINE Option<size_t> operator()(int x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<unsigned int> {
		PEFF_FORCEINLINE Option<size_t> operator()(unsigned int x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<long> {
		PEFF_FORCEINLINE Option<size_t> operator()(long x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<unsigned long> {
		PEFF_FORCEINLINE Option<size_t> operator()(unsigned long x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<long long> {
		PEFF_FORCEINLINE Option<size_t> operator()(long long x) const {
			return (size_t)+x;
		}
	};

	template <>
	struct FallibleHasher<unsigned long long> {
		PEFF_FORCEINLINE Option<size_t> operator()(unsigned long long x) const {
			return (size_t)+x;
		}
	};
}

#endif
