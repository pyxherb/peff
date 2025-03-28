#ifndef _PEFF_ADVUTILS_ADVUTILSDEFS_H_
#define _PEFF_ADVUTILS_ADVUTILSDEFS_H_

#include <peff/base/basedefs.h>

#if PEFF_DYNAMIC_LINK
	#if IS_PEFF_ADVUTILS_BUILDING
		#define PEFF_ADVUTILS_API PEFF_DLLEXPORT
	#else
		#define PEFF_ADVUTILS_API PEFF_DLLIMPORT
	#endif
#else
	#define PEFF_ADVUTILS_API
#endif

namespace peff {
}

#endif
