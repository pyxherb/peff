#include "rcobj.h"

using namespace peff;

PEFF_BASE_API std::atomic_size_t peff::g_rcObjectPtrCounter = 0;

PEFF_BASE_API RcObject::RcObject() noexcept: refCount(0) {}
PEFF_BASE_API RcObject::~RcObject() {}

PEFF_BASE_API void RcObject::onIncRef(size_t counter) {

}

PEFF_BASE_API void RcObject::onDecRef(size_t counter) {

}
