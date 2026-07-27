#ifndef _PEFF_BASE_BASEDEFS_H_
#define _PEFF_BASE_BASEDEFS_H_

#include <peff/generated/config.h>
#include <cstddef>
#include <cassert>

#if defined(_MSC_VER)
	#define PEFF_DLLEXPORT __declspec(dllexport)
	#define PEFF_DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_DLLEXPORT __attribute__((__visibility__("default")))
	#define PEFF_DLLIMPORT __attribute__((__visibility__("default")))
#endif

#if defined(_MSC_VER)
	#define PEFF_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#ifdef NDEBUG
		#define PEFF_FORCEINLINE __attribute__((__always_inline__)) inline
	#else
		#define PEFF_FORCEINLINE inline
	#endif
#endif

#if defined(_MSC_VER)
	#define PEFF_ASSUME(...) \
		assert(__VA_ARGS__); \
		__assume(__VA_ARGS__)
#elif defined(__GNUC__)
	#define PEFF_ASSUME(...) \
		if (__VA_ARGS__)     \
			;                \
		else                 \
			__builtin_assume(false)
#elif defined(__clang__)
	#define PEFF_ASSUME(...) \
		assert(__VA_ARGS__); \
		__builtin_assume(__VA_ARGS__)
#else
	#define PEFF_ASSUME(...) assert(__VA_ARGS__)
#endif

#if defined(_MSC_VER)
	#define PEFF_UNREACHABLE() \
		assert(false);            \
		__assume(false);
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_UNREACHABLE() \
		assert(false);            \
		__builtin_unreachable()
#else
	#define PEFF_UNREACHABLE() std::terminate()
#endif

#if defined(_MSC_VER)
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		api_modifier extern template class name<__VA_ARGS__>;
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		api_modifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		extern template class api_modifier name<__VA_ARGS__>;
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...)
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(api_modifier, name, ...)
#endif

#define PEFF_OFFSETOF(t, m) ((size_t)(&reinterpret_cast<const volatile char &>(static_cast<const t *>(nullptr)->m)))
#define PEFF_CONTAINER_OF(t, m, p) ((t *)(((char *)p) - PEFF_OFFSETOF(t, m)))

#ifdef _MSC_VER
	#define PEFF_RESTRICT __restrict
	#define PEFF_RESTRICT_PTR(type, name) type *__restrict name
	#define PEFF_RESTRICT_REF(type, name) type &__restrict name
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_RESTRICT __restrict__
	#define PEFF_RESTRICT_PTR(type, name) type *__restrict__ name
	#define PEFF_RESTRICT_REF(type, name) type &__restrict__ name
#else
	#define PEFF_RESTRICT
	#define PEFF_RESTRICT_PTR(type, name) type *name
	#define PEFF_RESTRICT_REF(type, name) type &name
#endif

#if PEFF_DYNAMIC_LINK
	#if IS_PEFF_BASE_BUILDING
		#define PEFF_BASE_API PEFF_DLLEXPORT
	#else
		#define PEFF_BASE_API PEFF_DLLIMPORT
	#endif
#else
	#define PEFF_BASE_API
#endif

#if __cplusplus < 201703L
	#error Your compiler/configuration/environment does not meet the requirement of PEFF
#endif

#if __cplusplus >= 202002L
	#define PEFF_REQUIRES_CONCEPT(...) requires __VA_ARGS__
#else
	#define PEFF_REQUIRES_CONCEPT(...)
#endif

#ifdef _MSC_VER
	#include <sal.h>
#endif

// The parameter must be null.
#ifdef _MSC_VER
	#define PEFF_MUSTNULL _Null_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_MUSTNULL
#else
	#define PEFF_MUSTNULL
#endif

// The parameter is nullable.
#ifdef _MSC_VER
	#define PEFF_NULLABLE _Maybenull_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_NULLABLE
#else
	#define PEFF_NULLABLE
#endif

// The parameter must be non-null.
#ifdef _MSC_VER
	#define PEFF_NONNULL _Notnull_
#elif defined(__clang__)
	#define PEFF_NONNULL __attribute__((nonnull))
#elif defined(__GNUC__)
	#define PEFF_NONNULL
#else
	#define PEFF_NONNULL
#endif

// The return value must be null.
#ifdef _MSC_VER
	#define PEFF_MUSTNULL_RET _Ret_null_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_MUSTNULL_RET
#else
	#define PEFF_MUSTNULL_RET
#endif

// The return value may be null.
#ifdef _MSC_VER
	#define PEFF_NULLABLE_RET _Ret_maybenull_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_NULLABLE_RET
#else
	#define PEFF_NULLABLE_RET
#endif

// The return value must not be null.
#ifdef _MSC_VER
	#define PEFF_NONNULL_RET _Ret_notnull_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_NONNULL_RET __attribute__((__returns_nonnull__))
#else
	#define PEFF_NONNULL_RET
#endif

// Specified parameter is non-nullable.
#ifdef _MSC_VER
	#define PEFF_NONNULL_ARG(index)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_NONNULL_ARG(index) __attribute__((__nonnull__(index)))
#else
	#define PEFF_NONNULL_ARG
#endif

// The parameter is a non-null input parameter.
#ifdef _MSC_VER
	#define PEFF_IN _In_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN PEFF_NONNULL
#else
	#define PEFF_IN
#endif

// The parameter is a nullable input parameter.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE _In_opt_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE
#else
	#define PEFF_IN_NULLABLE
#endif

// The parameter is a non-null output parameter.
#ifdef _MSC_VER
	#define PEFF_OUT _Out_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT PEFF_NONNULL
#else
	#define PEFF_OUT
#endif

// The parameter is a nullable output parameter.
#ifdef _MSC_VER
	#define PEFF_OUT_NULLABLE _Out_opt_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_NULLABLE
#else
	#define PEFF_OUT_NULLABLE
#endif

// The parameter is both a non-null input and a non-null output parameter.
#ifdef _MSC_VER
	#define PEFF_IN_OUT _Inout_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_OUT PEFF_NONNULL
#else
	#define PEFF_IN_OUT
#endif

// The parameter is both a nullable input and a non-null output parameter.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_OUT _Outptr_opt_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_OUT
#else
	#define PEFF_IN_NULLABLE_OUT
#endif

// The parameter is both a non-null input and a nullable output parameter.
#ifdef _MSC_VER
	#define PEFF_IN_OUT_NULLABLE _Outptr_result_maybenull_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_OUT_NULLABLE PEFF_NONNULL
#else
	#define PEFF_IN_OUT_NULLABLE
#endif

// The parameter is both a nullable input and a nullable output parameter.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_OUT_NULLABLE _Outptr_opt_result_maybenull_
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_OUT_NULLABLE
#else
	#define PEFF_IN_NULLABLE_OUT_NULLABLE
#endif

// The parameter is an input array.
#ifdef _MSC_VER
	#define PEFF_IN_ARRAY(size_arg) _In_reads_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_ARRAY(size_arg) PEFF_NONNULL
#else
	#define PEFF_IN_ARRAY(size_arg)
#endif

// The parameter is a nullable input array.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_ARRAY(size_arg) _In_reads_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_ARRAY(size_arg)
#else
	#define PEFF_IN_NULLABLE_ARRAY(size_arg)
#endif

// The parameter is an output array.
#ifdef _MSC_VER
	#define PEFF_OUT_ARRAY(size_arg) _Out_writes_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_ARRAY(size_arg) PEFF_NONNULL
#else
	#define PEFF_OUT_ARRAY(size_arg)
#endif

// The parameter is a nullable output parameter.
#ifdef _MSC_VER
	#define PEFF_OUT_NULLABLE_ARRAY(size_arg) _Out_writes_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_NULLABLE_ARRAY(size_arg)
#else
	#define PEFF_OUT_NULLABLE_ARRAY(size_arg)
#endif

// The parameter is both an input and an output array.
#ifdef _MSC_VER
	#define PEFF_IN_OUT_ARRAY(size_arg) _Inout_updates_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_OUT_ARRAY(size_arg) PEFF_NONNULL
#else
	#define PEFF_IN_OUT_ARRAY(size_arg)
#endif

// The parameter is a nullable input and output array.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_OUT_ARRAY(size_arg) _Inout_updates_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_OUT_ARRAY(size_arg)
#else
	#define PEFF_IN_NULLABLE_OUT_ARRAY(size_arg)
#endif

// The parameter is an input array.
#ifdef _MSC_VER
	#define PEFF_IN_ARRAY_BYTES(size_arg) _In_reads_bytes_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_ARRAY_BYTES(size_arg) PEFF_NONNULL
#else
	#define PEFF_IN_ARRAY_BYTES(size_arg)
#endif

// The parameter is a nullable input array.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_ARRAY_BYTES(size_arg) _In_reads_bytes_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_ARRAY_BYTES(size_arg)
#else
	#define PEFF_IN_NULLABLE_ARRAY_BYTES(size_arg)
#endif

// The parameter is an output array.
#ifdef _MSC_VER
	#define PEFF_OUT_ARRAY_BYTES(size_arg) _Out_writes_bytes_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_ARRAY_BYTES(size_arg) PEFF_NONNULL
#else
	#define PEFF_OUT_ARRAY_BYTES(size_arg)
#endif

// The parameter is a nullable output parameter.
#ifdef _MSC_VER
	#define PEFF_OUT_NULLABLE_ARRAY_BYTES(size_arg) _Out_writes_bytes_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_NULLABLE_ARRAY_BYTES(size_arg)
#else
	#define PEFF_OUT_NULLABLE_ARRAY_BYTES(size_arg)
#endif

// The parameter is both an input and an output array.
#ifdef _MSC_VER
	#define PEFF_IN_OUT_ARRAY_BYTES(size_arg) _Inout_updates_bytes_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_OUT_ARRAY_BYTES(size_arg) PEFF_NONNULL
#else
	#define PEFF_IN_OUT_ARRAY_BYTES(size_arg)
#endif

// The parameter is a nullable input and output array.
#ifdef _MSC_VER
	#define PEFF_IN_NULLABLE_OUT_ARRAY_BYTES(size_arg) _Inout_updates_bytes_opt_(size_arg)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_NULLABLE_OUT_ARRAY_BYTES(size_arg)
#else
	#define PEFF_IN_NULLABLE_OUT_ARRAY_BYTES(size_arg)
#endif

// Specified parameter is an input parameter.
#ifdef _MSC_VER
	#define PEFF_IN_ARG(index)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_ARG(index) __attribute__((__access__(read_only, index)))
#else
	#define PEFF_IN_ARG(index)
#endif

// Specified parameter is an output parameter.
#ifdef _MSC_VER
	#define PEFF_OUT_ARG(index)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_OUT_ARG(index) __attribute__((__access__(write_only, index)))
#else
	#define PEFF_OUT_ARG(index)
#endif

// Specified parameter is both an input and an output parameter.
#ifdef _MSC_VER
	#define PEFF_IN_OUT_ARG(index)
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_IN_OUT_ARG(index) __attribute__((__access__(read_write, index)))
#else
	#define PEFF_IN_OUT_ARG(index)
#endif

namespace peff {
}

#endif
