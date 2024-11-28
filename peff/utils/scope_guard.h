#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include <peff/base/basedefs.h>

namespace peff {
	template<typename T>
	struct ScopeGuard {
		T callback;
		bool released = false;

		ScopeGuard() = default;
		PEFF_FORCEINLINE ScopeGuard(T callback)
			: callback(callback) {
		}
		PEFF_FORCEINLINE ~ScopeGuard() {
			if (!released)
				callback();
		}

		PEFF_FORCEINLINE void release() noexcept {
			released = true;
		}
		PEFF_FORCEINLINE ScopeGuard& operator=(T callback) {
			this->callback = callback;
		}
	};
}

#endif
