#ifndef _PEFF_UTILS_OPTIONAL_H_
#define _PEFF_UTILS_OPTIONAL_H_

#include "basedefs.h"

namespace peff {
	template <typename T>
	struct Optional {
	private:
		T _value;
		bool _valid = false;

	public:
	};
}

#endif
