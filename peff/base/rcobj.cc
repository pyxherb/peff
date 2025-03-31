#include "rcobj.h"

using namespace peff;

PEFF_BASE_API RcObject::RcObject() noexcept: refCount(0) {}
PEFF_BASE_API RcObject::~RcObject() {}

PEFF_BASE_API void RcObject::_onRefZero() noexcept {
	{
		std::lock_guard weakPtrMutexGuard(weakPtrMutex);
		for (BaseWeakRcObjectPtr *i = weakPtrs, *next; i; i = next) {
			next = i->_next;
			i->_resetUnchecked();
		}
	}
	this->onRefZero();
}

PEFF_BASE_API void BaseWeakRcObjectPtr::_reset() {
	if (_ptr) {
		std::lock_guard weakPtrMutexGuard(_ptr->weakPtrMutex);
		_resetUnchecked();
	}
}

PEFF_BASE_API void BaseWeakRcObjectPtr::_resetUnchecked() {
	if (_prev)
		_prev->_next = _next;
	if (_next)
		_next->_prev = _prev;
	_ptr = nullptr;
}

PEFF_BASE_API void BaseWeakRcObjectPtr::_set(RcObject *ptr) {
	if (ptr) {
		std::lock_guard weakPtrMutexGuard(ptr->weakPtrMutex);
		_prev = nullptr;
		if ((_next = ptr->weakPtrs))
			_next->_prev = this;
		ptr->weakPtrs = this;
	}
	_ptr = ptr;
}

PEFF_BASE_API BaseWeakRcObjectPtr::BaseWeakRcObjectPtr(RcObject *ptr) {
	_set(ptr);
}

PEFF_BASE_API BaseWeakRcObjectPtr::~BaseWeakRcObjectPtr() {
	_reset();
}
