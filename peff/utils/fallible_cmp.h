#ifndef _PEFF_UTILS_FALLIBLE_CMP_H_
#define _PEFF_UTILS_FALLIBLE_CMP_H_

#include "basedefs.h"
#include "option.h"
#include <peff/base/alloc.h>
#include <peff/utils/option.h>

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

	template <typename T>
	struct FallibleCmpThreeway {
		Option<int> operator()(const T &lhs, const T &rhs) const {
#if __cplusplus >= 202002L
			return lhs <=> rhs;
#else
			if (lhs < rhs)
				return -1;
			else if (lhs > rhs)
				return 1;
			else
				return 0;
#endif
		}
	};
}

#endif
