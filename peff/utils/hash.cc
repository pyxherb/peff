#include "hash.h"

using namespace peff;

PEFF_UTILS_API uint32_t peff::djbHash32(const char* data, size_t size) {
	uint32_t hash = 5381;
	for (size_t i = 0; i < size; ++i) {
		hash += (hash << 5) + *(data++);
	}
	return hash;
}

PEFF_UTILS_API uint64_t djbHash64(const char *data, size_t size) {
	uint64_t hash = 5381;
	for (size_t i = 0; i < size; ++i) {
		hash += (hash << 5) + *(data++);
	}
	return hash;
}
