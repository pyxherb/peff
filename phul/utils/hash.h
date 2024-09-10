#ifndef _PHUL_UTILS_MISC_H_
#define _PHUL_UTILS_MISC_H_

#include <phul/base/allocator.h>

namespace phul {
	using HashCode = uint32_t;

	template <typename T>
	struct Hasher {
		static_assert(false, "Hasher for the type was not found");
	};

	template <>
	struct Hasher<signed char> {
		PHUL_FORCEINLINE HashCode operator()(signed char x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned char> {
		PHUL_FORCEINLINE HashCode operator()(unsigned char x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<char> {
		PHUL_FORCEINLINE HashCode operator()(char x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<short> {
		PHUL_FORCEINLINE HashCode operator()(short x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned short> {
		PHUL_FORCEINLINE HashCode operator()(unsigned short x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<int> {
		PHUL_FORCEINLINE HashCode operator()(int x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned int> {
		PHUL_FORCEINLINE HashCode operator()(unsigned int x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<long> {
		PHUL_FORCEINLINE HashCode operator()(long x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned long> {
		PHUL_FORCEINLINE HashCode operator()(unsigned long x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<long long> {
		PHUL_FORCEINLINE HashCode operator()(long long x) {
			return (HashCode)x;
		}
	};

	template <>
	struct Hasher<unsigned long> {
		PHUL_FORCEINLINE HashCode operator()(unsigned long long x) {
			return (HashCode)x;
		}
	};
}

#endif
