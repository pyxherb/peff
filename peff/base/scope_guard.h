#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include "basedefs.h"
#include <utility>
#include <type_traits>

namespace peff {
	/// @brief An RAII-based scope guard for executing codes automatically when leaving the scope.
	/// @tparam T For lambda expression, will be deduced by the compiler automatically.
	/// @note The lambda expression is required to be noexcept.
	template<typename T>
	struct ScopeGuard {
		T callback;
		bool released = false;

		static_assert(std::is_nothrow_invocable_v<T>);

		ScopeGuard() = delete;
		PEFF_FORCEINLINE ScopeGuard(T &&callback)
			: callback(std::move(callback)) {
		}
		PEFF_FORCEINLINE ~ScopeGuard() {
			if (!released)
				callback();
		}

		PEFF_FORCEINLINE void release() noexcept {
			released = true;
		}
	};
}

#endif
