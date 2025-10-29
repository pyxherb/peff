#ifndef _PEFF_UTILS_BYTEORD_H_
#define _PEFF_UTILS_BYTEORD_H_

#include "basedefs.h"

namespace peff {
	PEFF_UTILS_API extern bool _g_byteOrder;

	/// @brief Get current machine byte order.
	/// @return true The machine is in big-endian mode.
	/// @return false The machine is in little-endian mode.
	PEFF_FORCEINLINE bool getByteOrder() {
		return _g_byteOrder;
	}
}

#endif
