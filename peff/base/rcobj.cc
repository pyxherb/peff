#include "rcobj.h"

using namespace peff;

#if PEFF_ENABLE_RCOBJ_DEBUGGING
PEFF_BASE_API RcObjectPtrCounterResetEventListener *peff::g_rcobj_ptr_counter_reset_event_listener = nullptr;
PEFF_BASE_API std::atomic_size_t peff::g_rcobj_ptr_counter = 0;

PEFF_BASE_API RcObjectPtrCounterResetEventListener::RcObjectPtrCounterResetEventListener() {
}

PEFF_BASE_API RcObjectPtrCounterResetEventListener::~RcObjectPtrCounterResetEventListener() {
}

PEFF_BASE_API size_t peff::acquire_global_rcobj_ptr_counter() {
	return g_rcobj_ptr_counter++;
}

PEFF_BASE_API void peff::register_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener) {
	if (g_rcobj_ptr_counter_reset_event_listener) {
		g_rcobj_ptr_counter_reset_event_listener->_prev = listener;
	}

	g_rcobj_ptr_counter_reset_event_listener = listener;
}

PEFF_BASE_API void peff::unregister_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener) {
	if (listener == g_rcobj_ptr_counter_reset_event_listener) {
		g_rcobj_ptr_counter_reset_event_listener = listener->_next;
	}
	if (listener->_next)
		listener->_next->_prev = listener->_prev;
	if (listener->_prev)
		listener->_prev->_next = listener->_next;
}
#endif
