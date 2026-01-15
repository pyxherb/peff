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
	#define PEFF_UNREACHABLE(...) \
		assert(false);            \
		__assume(false);
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_UNREACHABLE(...) \
		assert(false);            \
		__builtin_unreachable()
#else
	#define PEFF_UNREACHABLE(...) std::terminate()
#endif

#if defined(_MSC_VER)
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier extern template class name<__VA_ARGS__>;
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		extern template class apiModifier name<__VA_ARGS__>;
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define PEFF_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
	#define PEFF_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
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

namespace peff {
}

#endif
