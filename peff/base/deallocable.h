#ifndef _PEFF_BASE_DEALLOCABLE_H_
#define _PEFF_BASE_DEALLOCABLE_H_

#include "basedefs.h"
#include <type_traits>

namespace peff {
	template<typename T>
	struct DeallocableDeleter {
		PEFF_FORCEINLINE void operator()(T *ptr) {
			if (ptr)
				ptr->dealloc();
		}
	};
}

#endif
