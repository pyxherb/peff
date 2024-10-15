#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include <peff/base/basedefs.h>

namespace peff {
	template<typename T>
	struct ScopeGuard {
		T callable;
		bool released = false;

		PEFF_FORCEINLINE ScopeGuard(T&& callable)
			: callable(callable) {
		}
		PEFF_FORCEINLINE ~ScopeGuard() {
			if (!released)
				callable();
		}

		PEFF_FORCEINLINE void release() noexcept {
			released = true;
		}
	};
}

#endif
