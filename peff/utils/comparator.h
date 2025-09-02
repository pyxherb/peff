#ifndef _PEFF_UTILS_COMPARATOR_H_
#define _PEFF_UTILS_COMPARATOR_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <optional>

namespace peff {
	template <typename T>
	struct FallibleLt {
		std::optional<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs < rhs;
		}
	};

	template <typename T>
	struct FallibleGt {
		std::optional<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs > rhs;
		}
	};

	template <typename T>
	struct FallibleEq {
		std::optional<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs == rhs;
		}
	};
}

#endif
