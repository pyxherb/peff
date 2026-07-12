#ifndef _PEFF_UTILS_SCOPE_GUARD_H_
#define _PEFF_UTILS_SCOPE_GUARD_H_

#include "basedefs.h"
#include <utility>
#include <type_traits>

namespace peff {
	/// @brief An RAII-based scope guard for executing codes automatically when leaving the scope. This guard is cancellable with the @c release() method.
	/// @tparam T For lambda expression, which should be inferred by the compiler automatically.
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

	/// @brief An RAII-based scope guard that will call the guard function automatically at the end of the scope. This guard will always be triggered and cannot be cancelled.
	/// @tparam T For the guard function, which should be inferred by the compiler automatically.
	/// @note The callback is required to be noexcept.
	template <typename T>
	struct Deferred {
		T callback;

		static_assert(std::is_nothrow_invocable_v<T>, "The callback must be noexcept");

		Deferred() = delete;
		PEFF_FORCEINLINE Deferred(T &&callback)
			: callback(std::move(callback)) {
		}
		PEFF_FORCEINLINE ~Deferred() {
			callback();
		}
	};
}

#endif
