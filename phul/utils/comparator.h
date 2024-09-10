#ifndef _PHUL_UTILS_COMPARATOR_H_
#define _PHUL_UTILS_COMPARATOR_H_

#include <phul/base/allocator.h>

namespace phul {
	template <typename T>
	struct Comparator {
		PHUL_FORCEINLINE int operator()(const T &lhs, const T &rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}
	};

	template <typename T>
	struct LtComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs < rhs;
		}
	};

	template <typename T>
	struct GtComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs > rhs;
		}
	};

	template <typename T>
	struct EqComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs == rhs;
		}
	};
}

#endif
