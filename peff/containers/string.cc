#include "string.h"

using namespace peff;

PEFF_CONTAINERS_API bool operator==(const peff::String& lhs, const peff::String& rhs) noexcept {
	if (lhs.size() != rhs.size())
		return false;

	return memcmp(lhs.data(), rhs.data(), lhs.size());
}
