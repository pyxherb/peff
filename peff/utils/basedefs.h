#ifndef _PEFF_UTILS_BASEDEFS_H_
#define _PEFF_UTILS_BASEDEFS_H_

#include <peff/base/basedefs.h>

#if IS_PEFF_UTILS_BUILDING
	#define PEFF_UTILS_API PEFF_DLLEXPORT
#else
	#define PEFF_UTILS_API PEFF_DLLIMPORT
#endif

#endif
