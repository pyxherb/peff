#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include <peff/base/basedefs.h>
#include <functional>

namespace peff {
	struct ScopeGuard {
		std::function<void()> callback;

		ScopeGuard() = default;
		PEFF_FORCEINLINE ScopeGuard(std::function<void()> &&callback)
			: callback(callback) {
		}
		PEFF_FORCEINLINE ~ScopeGuard() {
			if (callback)
				callback();
		}

		PEFF_FORCEINLINE void release() noexcept {
			callback = {};
		}
		PEFF_FORCEINLINE ScopeGuard& operator=(std::function<void()>&& callback) {
			this->callback = callback;
		}
	};
}

#endif
