#include "rcobj.h"

using namespace peff;

PEFF_BASE_API RcObject::RcObject() noexcept: refCount(0) {}
PEFF_BASE_API RcObject::~RcObject() {}
