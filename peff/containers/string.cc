#include "string.h"
#include <cctype>

using namespace peff;

PEFF_CONTAINERS_API void String::to_upper() noexcept {
	const size_t len = size();
	for (size_t i = 0; i < len; ++i) {
		char &c = _array.at(i);
		if (islower(c))
			c = toupper(c);
	}
}

PEFF_CONTAINERS_API void String::to_lower() noexcept {
	const size_t len = size();
	for (size_t i = 0; i < len; ++i) {
		char &c = _array.at(i);
		if (isupper(c))
			c = tolower(c);
	}
}
