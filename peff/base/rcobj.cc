#include "rcobj.h"

using namespace peff;

PEFF_BASE_API RcObject::RcObject() noexcept {}
PEFF_BASE_API RcObject::~RcObject() {}

PEFF_BASE_API void RcObject::onRefZero() noexcept {
	delete this;
}
