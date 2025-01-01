#include "misc.h"

using namespace peff;

static uint16_t _s_nativeEndianTester = 0xff00;

PEFF_UTILS_API Endian peff::testNativeEndian() {
	return *((uint8_t *)&_s_nativeEndianTester) ? Endian::Big : Endian::Little;
}
