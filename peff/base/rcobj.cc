#include "rcobj.h"

using namespace peff;

PEFF_BASE_API RcObject::RcObject() noexcept {}
PEFF_BASE_API RcObject::~RcObject() {}

PEFF_BASE_API void RcObject::_onRefZero() noexcept {
	{
		weakPtrMutex.lock();
		if(refCount)
			return;
		for (BaseWeakRcObjectPtr *i = weakPtrs, *next; i; i = next) {
			next = i->_next;
			i->_resetUnchecked();
		}
		weakPtrMutex.unlock();
	}
	this->onRefZero();
}

PEFF_BASE_API void BaseWeakRcObjectPtr::_reset() {
	if (_ptr) {
		_ptr->weakPtrMutex.lock();
		if (_prev)
			_prev->_next = _next;
		if (_next)
			_next->_prev = _prev;
		_ptr->weakPtrMutex.unlock();
	}
	_ptr = nullptr;
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
		ptr->weakPtrMutex.lock();
		_next = ptr->weakPtrs;
		if (ptr->weakPtrs)
			_next->_prev = this;
		ptr->weakPtrs = this;
		_prev = nullptr;
		ptr->weakPtrMutex.unlock();
	}
	_ptr = ptr;
}

PEFF_BASE_API BaseWeakRcObjectPtr::BaseWeakRcObjectPtr(RcObject *ptr) {
	_set(ptr);
}

PEFF_BASE_API BaseWeakRcObjectPtr::~BaseWeakRcObjectPtr() {
	_reset();
}
