#ifndef _PEFF_UTILS_COMPARATOR_H_
#define _PEFF_UTILS_COMPARATOR_H_

#include <peff/base/allocator.h>

namespace peff {
	template <typename T>
	struct Comparator {
		PEFF_FORCEINLINE int operator()(const T &lhs, const T &rhs) {
			if (lhs < rhs)
				return -1;
			if (lhs > rhs)
				return 1;
			return 0;
		}

		PEFF_FORCEINLINE bool copy(Comparator<T> &dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(Comparator<T> &dest) const {
			return true;
		}
	};

	template <typename T>
	struct LtComparator {
		PEFF_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs < rhs;
		}

		PEFF_FORCEINLINE bool copy(LtComparator<T> &dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(LtComparator<T> &dest) const {
			return true;
		}
	};

	template <typename T>
	struct GtComparator {
		PEFF_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs > rhs;
		}

		PEFF_FORCEINLINE bool copy(GtComparator<T> &dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(GtComparator<T> &dest) const {
			return true;
		}
	};


	template <typename T>
	struct EqComparator {
		PEFF_FORCEINLINE bool operator()(const T &lhs, const T &rhs) {
			return lhs == rhs;
		}

		PEFF_FORCEINLINE bool copy(EqComparator<T> &dest) const {
			return true;
		}
		PEFF_FORCEINLINE bool copyAssign(EqComparator<T> &dest) const {
			return true;
		}
	};
}

#endif
