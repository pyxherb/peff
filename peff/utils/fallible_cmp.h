#ifndef _PEFF_UTILS_FALLIBLE_CMP_H_
#define _PEFF_UTILS_FALLIBLE_CMP_H_

#include "basedefs.h"
#include "option.h"
#include <peff/base/alloc.h>
#include <peff/utils/misc.h>
#include <peff/utils/option.h>

#if PEFF_CPLUSPLUS >= 202002L
	#include <compare>
#endif

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

	/// @brief A fallible three-way comparator that yields a peff::PartialOrd.
	template <typename T>
	struct FallibleCmpThreeway {
		/// @brief Compare two operands in a fallible three-way manner.
		/// @param lhs The left-hand operand.
		/// @param rhs The right-hand operand.
		/// @return peff::PARTIAL_LT, peff::PARTIAL_GT or peff::PARTIAL_EQ according to the operands, or peff::PARTIAL_UNORDERED when they are not comparable.
		PartialOrd operator()(const T &lhs, const T &rhs) const {
#if PEFF_CPLUSPLUS >= 202002L
			const std::partial_ordering result = lhs <=> rhs;

			if (result < 0)
				return PARTIAL_LT;
			if (result > 0)
				return PARTIAL_GT;
			if (result == 0)
				return PARTIAL_EQ;

			return PARTIAL_UNORDERED;
#else
			if (lhs < rhs)
				return PARTIAL_LT;
			if (lhs > rhs)
				return PARTIAL_GT;
			if (lhs == rhs)
				return PARTIAL_EQ;

			return PARTIAL_UNORDERED;
#endif
		}
	};
}

#endif
