#ifndef _PEFF_UTILS_HASH_H_
#define _PEFF_UTILS_HASH_H_

#include "basedefs.h"
#include <string_view>
#include <peff/base/alloc.h>
#include <optional>

namespace peff {
	template <typename T>
	struct Hasher {
		static_assert(!std::false_type::value, "Hasher not found");
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

	PEFF_UTILS_API uint32_t djb_hash32(const char *data, size_t size);
	PEFF_UTILS_API uint64_t djb_hash64(const char *data, size_t size);
	PEFF_UTILS_API uint32_t city_hash32(const char *s, size_t len); // FIXME: Fix hash inequality bug.
	PEFF_UTILS_API uint64_t city_hash64(const char *s, size_t len); // FIXME: Fix hash inequality bug.

	template <>
	struct Hasher<std::string_view> {
		PEFF_FORCEINLINE std::conditional_t<sizeof(size_t) <= sizeof(uint32_t), uint32_t, uint64_t> operator()(const std::string_view &x) const {
			if constexpr (sizeof(size_t) <= sizeof(uint32_t)) {
				return city_hash32(x.data(), x.size());
			} else {
				return city_hash64(x.data(), x.size());
			}
		}
	};
}

#endif
