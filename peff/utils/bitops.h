#ifndef _PEFF_UTILS_BITOPS_H_
#define _PEFF_UTILS_BITOPS_H_

#include "basedefs.h"
#include <cstring>
#include <cstdint>
#include <type_traits>

#if __cplusplus >= 202002L
	#include <bit>
#endif

#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		#include <intrin.h>
	#elif defined(__GNUC__) || defined(__clang__)
	#endif
#endif

namespace peff {
	template <typename To, typename From>
	PEFF_FORCEINLINE To bitCast(const From &from) noexcept {
#if __cplusplus >= 202002L
		return std::bit_cast<To>(std::forward<const From &>(from));
#else
		static_assert(sizeof(To) == sizeof(From), "The size of two types must be equal");
		static_assert(std::is_trivially_copyable_v<To>, "The destination type must be trivially copyable");
		static_assert(std::is_trivially_copyable_v<From>, "The source type must be trivially copyable");
		static_assert(std::is_trivially_constructible_v<To>, "The destination type must be trivially constructible");
		To dest;
		memcpy(&dest, &from, sizeof(To));
		return dest;
#endif
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(uint8_t value) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#if defined(_MSC_VER)
		if (!value)
			return 8;
		return _lzcnt_u32((uint32_t)value) - (sizeof(uint32_t) - sizeof(uint8_t)) * 8;
	#elif defined(__GNUC__) || defined(__clang__)
		if (!value)
			return 8;
		return __builtin_clz((uint32_t)value);
	#else
		#define _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U8 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U8
		if (!value)
			return 8;
		uint8_t cnt = 0;
		value = ~value;
		while (value & 0x80) {
			value <<= 1;
			++cnt;
		}
		return cnt;
#endif
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(uint16_t value) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		if (!value)
			return 16;
		return _lzcnt_u32((uint32_t)value) - (sizeof(uint32_t) - sizeof(uint16_t)) * 8;
	#elif defined(__GNUC__) || defined(__clang__)
		if (!value)
			return 16;
		return __builtin_clz((uint32_t)value);
	#else
		#define _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U16 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U16
		if (!value)
			return 16;
		uint8_t cnt = 0;
		value = ~value;
		while (value & 0x8000) {
			value <<= 1;
			++cnt;
		}
		return cnt;
#endif
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(uint32_t value) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		if (!value)
			return 32;
		return _lzcnt_u32((uint32_t)value);
	#elif defined(__GNUC__) || defined(__clang__)
		if (!value)
			return 32;
		return __builtin_clz((uint32_t)value);
	#else
		#define _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U32 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U32
		if (!value)
			return 32;
		uint8_t cnt = 0;
		value = ~value;
		while (value & 0x80000000U) {
			value <<= 1;
			++cnt;
		}
		return cnt;
#endif
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(uint64_t value) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		if (!value)
			return 64;
		return _lzcnt_u64((uint32_t)value);
	#elif defined(__GNUC__) || defined(__clang__)
		if (!value)
			return 64;
		return __builtin_clz((uint32_t)value);
	#else
		#define _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U64 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_COUNT_LEADING_ZERO_U64
		if (!value)
			return 64;
		uint8_t cnt = 0;
		value = ~value;
		while (value & 0x8000000000000000ULL) {
			value <<= 1;
			++cnt;
		}
		return cnt;
#endif
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(int8_t value) {
		return countLeadingZero((uint8_t)value);
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(int16_t value) {
		return countLeadingZero((uint16_t)value);
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(int32_t value) {
		return countLeadingZero((uint32_t)value);
	}

	PEFF_FORCEINLINE uint8_t countLeadingZero(int64_t value) {
		return countLeadingZero((uint64_t)value);
	}

	PEFF_FORCEINLINE uint8_t rRot(uint8_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSVC
		return _rotr16(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U8 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U8
		return shift ? ((value >> shift) | (value << (8 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint16_t rRot(uint16_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotr16(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U16 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U16
		return shift ? ((value >> shift) | (value << (16 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint32_t rRot(uint32_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotr(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U32 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U32
		return shift ? ((value >> shift) | (value << (32 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint64_t rRot(uint64_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotr64(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U64 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U64
		return shift ? ((value >> shift) | (value << (64 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE int8_t rRot(int8_t value, uint_fast8_t shift) {
		return rRot((uint8_t)value, shift);
	}

	PEFF_FORCEINLINE int16_t rRot(int16_t value, uint_fast8_t shift) {
		return rRot((uint16_t)value, shift);
	}

	PEFF_FORCEINLINE int32_t rRot(int32_t value, uint_fast8_t shift) {
		return rRot((uint32_t)value, shift);
	}

	PEFF_FORCEINLINE int64_t rRot(int64_t value, uint_fast8_t shift) {
		return rRot((uint64_t)value, shift);
	}

	PEFF_FORCEINLINE uint8_t lRot(uint8_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotl16(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_LROT_U8 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_LROT_U8
		return shift ? ((value << shift) | (value >> (8 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint16_t lRot(uint16_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotl16(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_LROT_U16 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_LROT_U16
		return shift ? ((value << shift) | (value >> (16 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint32_t lRot(uint32_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotl(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U32 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U32
		return shift ? ((value << shift) | (value >> (32 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE uint64_t lRot(uint64_t value, uint_fast8_t shift) {
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__)
	#ifdef _MSC_VER
		return _rotl64(value, shift);
	#else
		#define _PEFF_USE_DEFAULT_RROT_U64 1
	#endif
#endif
#if _PEFF_USE_DEFAULT_RROT_U64
		return shift ? ((value << shift) | (value >> (32 - shift))) : value;
#endif
	}

	PEFF_FORCEINLINE int8_t lRot(int8_t value, uint_fast8_t shift) {
		return lRot((uint8_t)value, shift);
	}

	PEFF_FORCEINLINE int16_t lRot(int16_t value, uint_fast8_t shift) {
		return lRot((uint16_t)value, shift);
	}

	PEFF_FORCEINLINE int32_t lRot(int32_t value, uint_fast8_t shift) {
		return lRot((uint32_t)value, shift);
	}

	PEFF_FORCEINLINE int64_t lRot(int64_t value, uint_fast8_t shift) {
		return lRot((uint64_t)value, shift);
	}
}

#endif
