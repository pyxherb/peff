#ifndef _PHUL_BASE_ALLOCATOR_H_
#define _PHUL_BASE_ALLOCATOR_H_

#include "traits.h"
#include <memory_resource>

namespace phul {
	class StdAlloc {
	public:
		struct AllocRecord {
			AllocRecord *prev, *next;
			size_t size;
		};

	private:
		PHUL_BASE_API static void _addAllocRecord(AllocRecord *allocRecord);
		PHUL_BASE_API static void _removeAllocRecord(AllocRecord *allocRecord);

	public:
		PHUL_BASE_API static AllocRecord *s_allocRecords;

		PHUL_BASE_API void *alloc(size_t size);
		PHUL_BASE_API void release(void *ptr);

		PHUL_FORCEINLINE bool copy(StdAlloc& dest) const {
			return true;
		}
	};

	class VoidAlloc {
	public:
		PHUL_FORCEINLINE void *alloc(size_t size) {
			throw std::logic_error("Cannot allocate memory by VoidAlloc");
		}
		PHUL_FORCEINLINE void release(void *ptr) {
			throw std::logic_error("Cannot free memory by VoidAlloc");
		}

		PHUL_FORCEINLINE bool copy(VoidAlloc &dest) {
			return true;
		}
	};

	class PmrAlloc {
	public:
		struct alignas(size_t) AllocRecord {
			AllocRecord *prev, *next;
			size_t size;
		};

	private:
		PHUL_BASE_API void _addAllocRecord(AllocRecord *allocReocrd);
		PHUL_BASE_API void _removeAllocRecord(AllocRecord *allocRecord);

	public:
		std::pmr::memory_resource *memoryResource = nullptr;
		AllocRecord *allocRecords = nullptr;

		PHUL_BASE_API void *alloc(size_t size);
		PHUL_BASE_API void release(void *ptr);
	};
}

#endif
