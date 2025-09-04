#ifndef _PEFF_CONTAINERS_FALLIBLE_CMP_H_
#define _PEFF_CONTAINERS_FALLIBLE_CMP_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include "option.h"

namespace peff {
	template <typename T>
	struct FallibleLt {
		Option<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs < rhs;
		}
	};

	template <typename T>
	struct FallibleGt {
		Option<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs > rhs;
		}
	};

	template <typename T>
	struct FallibleEq {
		Option<bool> operator()(const T &lhs, const T &rhs) const {
			return lhs == rhs;
		}
	};
}

#endif
