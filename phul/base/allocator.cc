#include "allocator.h"
#include <cstdlib>

using namespace phul;

PHUL_BASE_API void *StdAlloc::alloc(size_t size) {
	return malloc(size);
}

PHUL_BASE_API void StdAlloc::release(void *ptr) {
	free(ptr);
}

PHUL_BASE_API void PmrAlloc::_addAllocRecord(AllocRecord *allocRecord) {
	if (!allocRecords) {
		allocRecords = allocRecord;
		return;
	}

	allocRecord->next = allocRecords;
	allocRecords->prev = allocRecord;
	allocRecords = allocRecord;
}

PHUL_BASE_API void PmrAlloc::_removeAllocRecord(AllocRecord *allocRecord) {
	if (allocRecord->prev)
		allocRecord->prev->next = allocRecord->next;
	if (allocRecord->next)
		allocRecord->next->prev = allocRecord->prev;
	if (allocRecord == allocRecords)
		allocRecords = allocRecord->next;
}

PHUL_BASE_API void *PmrAlloc::alloc(size_t size) {
	if (!memoryResource)
		throw std::logic_error("Memory resource has not been set yet");

	char *ptr = (char *)memoryResource->allocate(sizeof(AllocRecord) + size, 1);
	if (!ptr)
		return nullptr;

	{
		AllocRecord *allocRecord = (AllocRecord*)ptr;

		allocRecord->size = size;
		_addAllocRecord(allocRecord);
	}

	return ptr + sizeof(AllocRecord);
}