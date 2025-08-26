#ifndef _PEFF_BASE_BASEDEFS_H_
#define _PEFF_BASE_BASEDEFS_H_

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
	#ifndef NDEBUG
		#define PEFF_FORCEINLINE __attribute__((__always_inline__)) inline
	#else
		#define PEFF_FORCEINLINE inline
	#endif
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

#define PEFF_CONTAINER_OF(t, m, p) ((t *)(((char *)p) - offsetof(t, m)))

#if PEFF_DYNAMIC_LINK
	#if IS_PEFF_BASE_BUILDING
		#define PEFF_BASE_API PEFF_DLLEXPORT
	#else
		#define PEFF_BASE_API PEFF_DLLIMPORT
	#endif
#else
	#define PEFF_BASE_API
#endif

#ifdef _MSC_VER

	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#include <cstdlib>

	// Memory leak detection
	#define malloc(n) _malloc_dbg(n, _NORMAL_BLOCK, __FILE__, __LINE__)
	#define free(n) _free_dbg(n, _NORMAL_BLOCK)
	#define __DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
	#define new __DBG_NEW

#endif

#if __cplusplus < 201703L
	#error Your compiler/configuration/environment does not meet the requirement of PEFF
#endif

namespace peff {
}

#endif
