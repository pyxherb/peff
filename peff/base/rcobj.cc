#include "rcobj.h"

using namespace peff;

PEFF_BASE_API RcObjectPtrCounterResetEventListener *peff::g_rcObjectPtrCounterResetEventListeners = nullptr;
PEFF_BASE_API std::atomic_size_t peff::g_rcObjectPtrCounter = 0;

PEFF_BASE_API RcObjectPtrCounterResetEventListener::RcObjectPtrCounterResetEventListener() {
}

PEFF_BASE_API RcObjectPtrCounterResetEventListener::~RcObjectPtrCounterResetEventListener() {
}

PEFF_BASE_API size_t peff::acquireGlobalRcObjectPtrCounter() {
	return g_rcObjectPtrCounter++;
}

PEFF_BASE_API void peff::registerRcObjectPtrCounterResetEventListener(RcObjectPtrCounterResetEventListener* listener) {
	if (g_rcObjectPtrCounterResetEventListeners) {
		g_rcObjectPtrCounterResetEventListeners->_prev = listener;
	}

	g_rcObjectPtrCounterResetEventListeners = listener;
}

PEFF_BASE_API void peff::unregisterRcObjectPtrCounterResetEventListener(RcObjectPtrCounterResetEventListener* listener) {
	if (listener == g_rcObjectPtrCounterResetEventListeners) {
		g_rcObjectPtrCounterResetEventListeners = listener->_next;
	}
	if (listener->_next)
		listener->_next->_prev = listener->_prev;
	if (listener->_prev)
		listener->_prev->_next = listener->_next;
}
