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

		PHUL_FORCEINLINE bool copy(Comparator<T> &dest) const {
			return true;
		}
		PHUL_FORCEINLINE bool copyAssign(Comparator<T> &&dest) const {
			return true;
		}
	};

	template <typename T>
	struct LtComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs < rhs;
		}

		PHUL_FORCEINLINE bool copy(LtComparator<T> &dest) const {
			return true;
		}
		PHUL_FORCEINLINE bool copyAssign(LtComparator<T> &&dest) const {
			return true;
		}
	};

	template <typename T>
	struct GtComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs > rhs;
		}

		PHUL_FORCEINLINE bool copy(GtComparator<T> &dest) const {
			return true;
		}
		PHUL_FORCEINLINE bool copyAssign(GtComparator<T> &&dest) const {
			return true;
		}
	};

	template <typename T>
	struct EqComparator {
		PHUL_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs == rhs;
		}

		PHUL_FORCEINLINE bool copy(EqComparator<T> &dest) const {
			return true;
		}
		PHUL_FORCEINLINE bool copyAssign(EqComparator<T> &&dest) const {
			return true;
		}
	};
}

#endif
