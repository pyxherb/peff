#include "pmr.h"

using namespace peff;

PEFF_BASE_API MemoryResource::~MemoryResource() {}

PEFF_BASE_API DefaultMemoryResource::DefaultMemoryResource() {
}

PEFF_BASE_API DefaultMemoryResource::~DefaultMemoryResource() {
}

PEFF_BASE_API void *DefaultMemoryResource::alloc(size_t size, size_t alignment) {
	return _allocator.alloc(size, alignment);
}

PEFF_BASE_API void DefaultMemoryResource::release(void *ptr) {
	_allocator.release(ptr);
}

PEFF_BASE_API void *MonotonicBufferMemoryResource::alloc(size_t size, size_t alignment) {
	size_t nextAlignmentDiff = alignment - (this->size % alignment);
	size_t alignedSize = size + ((alignment) - (size % alignment));
	size_t totalAllocSize = nextAlignmentDiff + alignedSize;
	size_t newSzAllocated = _szAllocated + totalAllocSize;

	if (newSzAllocated > size) {
		if (upstream)
			return upstream->alloc(size, alignment);
		return nullptr;
	}

	char *ptr = ((char *)buffer) + nextAlignmentDiff;
	_szAllocated = newSzAllocated;

	return ptr;
}

PEFF_BASE_API void MonotonicBufferMemoryResource::release(void* ptr) {
}

PEFF_BASE_API void *PmrAlloc::alloc(size_t size, size_t alignment) {
	return memoryResource->alloc(size, alignment);
}

PEFF_BASE_API void PmrAlloc::release(void *ptr) {
	memoryResource->release(ptr);
}
