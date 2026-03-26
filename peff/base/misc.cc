#include "misc.h"

using namespace peff;

PEFF_BASE_API Endian peff::test_native_endian() {
	uint16_t _s_nativeEndianTester = 0xff00;
	return *(uint8_t *)&_s_nativeEndianTester ? Endian::Big : Endian::Little;
}
