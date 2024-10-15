#ifndef _PHUL_UTILS_SCOPE_GUARD_H_
#define _PHUL_UTILS_SCOPE_GUARD_H_

#include <phul/base/basedefs.h>

namespace phul {
	template<typename T>
	struct ScopeGuard {
		T callable;
		bool released = false;

		PHUL_FORCEINLINE ScopeGuard(T&& callable)
			: callable(callable) {
		}
		PHUL_FORCEINLINE ~ScopeGuard() {
			if (!released)
				callable();
		}

		PHUL_FORCEINLINE void release() noexcept {
			released = true;
		}
	};
}

#endif
