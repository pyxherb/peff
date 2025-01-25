#ifndef _PEFF_BASE_BASEDEFS_H_
#define _PEFF_BASE_BASEDEFS_H_

#if PEFF_DYNAMIC_LINK
	#if defined(_MSC_VER)
		#define PEFF_DLLEXPORT __declspec(dllexport)
		#define PEFF_DLLIMPORT __declspec(dllimport)
	#elif defined(__GNUC__) || defined(__clang__)
		#define PEFF_DLLEXPORT __attribute__((__visibility__("default")))
		#define PEFF_DLLIMPORT __attribute__((__visibility__("default")))
	#endif
#else
	#define PEFF_DLLEXPORT
	#define PEFF_DLLIMPORT
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

#if IS_PEFF_BASE_BUILDING
	#define PEFF_BASE_API PEFF_DLLEXPORT
#else
	#define PEFF_BASE_API PEFF_DLLIMPORT
#endif

namespace peff {
}

#endif
