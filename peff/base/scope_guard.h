#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include "basedefs.h"
#include <utility>
#include <type_traits>

namespace peff {
	/// @brief An RAII-based scope guard for executing codes automatically when leaving the scope.
	/// @tparam T For lambda expression, will be deduced by the compiler automatically.
	/// @note The callback is required to be noexcept.
	template <typename T>
	struct ScopeGuard {
		T callback;
		bool released = false;

		static_assert(std::is_nothrow_invocable_v<T>, "The callback must be noexcept");

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

	template <typename T>
	struct OneshotScopeGuard {
		T callback;

		static_assert(std::is_nothrow_invocable_v<T>, "The callback must be noexcept");

		OneshotScopeGuard() = delete;
		PEFF_FORCEINLINE OneshotScopeGuard(T &&callback)
			: callback(std::move(callback)) {
		}
		PEFF_FORCEINLINE ~OneshotScopeGuard() {
			callback();
		}
	};
}

#endif
