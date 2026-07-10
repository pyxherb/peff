#include "misc.h"

using namespace peff;

PEFF_BASE_API Endian peff::test_native_endian() {
	uint16_t e = 0xff00;
	uint8_t v;
	memcpy(&v, &e, sizeof(uint8_t));
	return v ? Endian::Big : Endian::Little;
}
