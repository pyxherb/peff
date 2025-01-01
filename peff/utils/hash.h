#ifndef _PEFF_UTILS_HASH_H_
#define _PEFF_UTILS_HASH_H_

#include <peff/base/allocator.h>

namespace peff {
	using HashCode = uint32_t;

	template <typename T>
	struct Hasher {
		static_assert(!std::is_same_v<T, T>, "Hasher for the type was not found");
	};

	template <>
	struct Hasher<signed char> {
		PEFF_FORCEINLINE HashCode operator()(signed char x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned char> {
		PEFF_FORCEINLINE HashCode operator()(unsigned char x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<char> {
		PEFF_FORCEINLINE HashCode operator()(char x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<short> {
		PEFF_FORCEINLINE HashCode operator()(short x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned short> {
		PEFF_FORCEINLINE HashCode operator()(unsigned short x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<int> {
		PEFF_FORCEINLINE HashCode operator()(int x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned int> {
		PEFF_FORCEINLINE HashCode operator()(unsigned int x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<long> {
		PEFF_FORCEINLINE HashCode operator()(long x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned long> {
		PEFF_FORCEINLINE HashCode operator()(unsigned long x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<long long> {
		PEFF_FORCEINLINE HashCode operator()(long long x) const {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned long long> {
		PEFF_FORCEINLINE HashCode operator()(unsigned long long x) const {
			return (HashCode)x;
		}
	};
}

#endif
