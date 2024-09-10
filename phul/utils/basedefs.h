#ifndef _PHUL_UTILS_BASEDEFS_H_
#define _PHUL_UTILS_BASEDEFS_H_

#include <phul/base/basedefs.h>

#if IS_PHUL_UTILS_BUILDING
	#define PHIL_UTILS_API PHUL_DLLEXPORT
#else
	#define PHIL_UTILS_API PHUL_DLLIMPORT
#endif

#endif
