#ifndef _PEFF_BASE_BASEDEFS_H_
#define _PEFF_BASE_BASEDEFS_H_

#if PEFF_DYNAMIC_LINK
	#if defined(_MSC_VER)
		#define PEFF_DLLEXPORT __declspec(dllexport)
		#define PEFF_DLLIMPORT __declspec(dllimport)
	#elif defined(__GNUC__) || defined(__clang__)
		#define PEFF_DLLEXPORT __visbility__((__default__))
		#define PEFF_DLLIMPORT __visbility__((__default__))
	#endif
#else
	#define PEFF_DLLEXPORT
	#define PEFF_DLLIMPORT
#endif

#if defined(_MSC_VER)
	#define PEFF_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define PEFF_FORCEINLINE __attribute__((__always_inline__)) inline
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

#if IS_PEFF_BASE_BUILDING
	#define PEFF_BASE_API PEFF_DLLEXPORT
#else
	#define PEFF_BASE_API PEFF_DLLIMPORT
#endif

namespace peff {
}

#endif
