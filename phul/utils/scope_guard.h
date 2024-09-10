#ifndef _PHUL_UTILS_SCOPE_GUARD_H_
#define _PHUL_UTILS_SCOPE_GUARD_H_

#include <phul/base/basedefs.h>
#include <functional>

namespace phul {
	struct ScopeGuard {
		std::function<void()> callable;

		PHUL_FORCEINLINE ScopeGuard(std::function<void()>&& callable)
			: callable(callable) {
		}
		PHUL_FORCEINLINE ~ScopeGuard() {
			if (callable)
				callable();
		}

		PHUL_FORCEINLINE void release() noexcept {
			callable = {};
		}
	};
}

#endif
