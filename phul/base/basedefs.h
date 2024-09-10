#ifndef _PHUL_BASE_BASEDEFS_H_
#define _PHUL_BASE_BASEDEFS_H_

#if defined(_MSC_VER)
	#define PHUL_DLLEXPORT __declspec(dllexport)
	#define PHUL_DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
	#define PHUL_DLLEXPORT __visbility__((__default__))
	#define PHUL_DLLIMPORT __visbility__((__default__))
#endif

#if defined(_MSC_VER)
	#define PHUL_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define PHUL_FORCEINLINE __attribute__((__always_inline__)) inline
#endif

#if defined(_MSC_VER)
	#define PHUL_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier extern template class name<__VA_ARGS__>;
	#define PHUL_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define PHUL_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		extern template class apiModifier name<__VA_ARGS__>;
	#define PHUL_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define PHUL_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
	#define PHUL_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
#endif

#if IS_PHUL_BASE_BUILDING
	#define PHUL_BASE_API PHUL_DLLEXPORT
#else
	#define PHUL_BASE_API PHUL_DLLIMPORT
#endif

namespace phul {
}

#endif
