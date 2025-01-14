#ifndef _PEFF_BASE_DEALLOCABLE_H_
#define _PEFF_BASE_DEALLOCABLE_H_

#include "basedefs.h"
#include <type_traits>

namespace peff {
	class Deallocable {
	public:
		PEFF_BASE_API Deallocable() noexcept;
		PEFF_BASE_API virtual ~Deallocable();

		virtual void dealloc() = 0;
	};

	struct DeallocableDeleter {
		PEFF_FORCEINLINE void operator()(Deallocable *ptr) {
			if (ptr)
				ptr->dealloc();
		}
	};
}

#endif
