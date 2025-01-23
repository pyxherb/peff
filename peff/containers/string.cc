#include "string.h"

using namespace peff;

PEFF_CONTAINERS_API bool operator==(const std::string_view& lhs, const peff::String& rhs) noexcept {
	return rhs == lhs;
}
