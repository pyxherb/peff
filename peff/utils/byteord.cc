#include "byteord.h"
#include <cstdint>

using namespace peff;

PEFF_UTILS_API bool peff::_g_byteOrder;

struct GenByteOrderConstructor {
	GenByteOrderConstructor() {
		uint16_t u = 0xff00;

		_g_byteOrder = *(uint8_t *)&u;
	}
};

static GenByteOrderConstructor _cons;
