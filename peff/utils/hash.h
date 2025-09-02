#ifndef _PEFF_UTILS_HASH_H_
#define _PEFF_UTILS_HASH_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <optional>

namespace peff {
	template <typename T>
	struct Hasher {
		static_assert(!std::is_same_v<T, T>, "Hasher was not found");
	};

	template <>
	struct Hasher<signed char> {
		PEFF_FORCEINLINE size_t operator()(signed char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<unsigned char> {
		PEFF_FORCEINLINE size_t operator()(unsigned char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<char> {
		PEFF_FORCEINLINE size_t operator()(char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<short> {
		PEFF_FORCEINLINE size_t operator()(short x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<unsigned short> {
		PEFF_FORCEINLINE size_t operator()(unsigned short x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<int> {
		PEFF_FORCEINLINE size_t operator()(int x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<unsigned int> {
		PEFF_FORCEINLINE size_t operator()(unsigned int x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<long> {
		PEFF_FORCEINLINE size_t operator()(long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<unsigned long> {
		PEFF_FORCEINLINE size_t operator()(unsigned long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<long long> {
		PEFF_FORCEINLINE size_t operator()(long long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct Hasher<unsigned long long> {
		PEFF_FORCEINLINE size_t operator()(unsigned long long x) const {
			return (size_t)x;
		}
	};

	template <typename T>
	struct FallibleHasher {
		static_assert(!std::is_same_v<T, T>, "Hasher was not found");
	};

	template <>
	struct FallibleHasher<signed char> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(signed char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<unsigned char> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(unsigned char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<char> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(char x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<short> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(short x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<unsigned short> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(unsigned short x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<int> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(int x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<unsigned int> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(unsigned int x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<long> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<unsigned long> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(unsigned long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<long long> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(long long x) const {
			return (size_t)x;
		}
	};

	template <>
	struct FallibleHasher<unsigned long long> {
		PEFF_FORCEINLINE std::optional<size_t> operator()(unsigned long long x) const {
			return (size_t)x;
		}
	};

	PEFF_UTILS_API uint32_t djbHash32(const char *data, size_t size);
	PEFF_UTILS_API uint64_t djbHash64(const char *data, size_t size);
}

#endif
