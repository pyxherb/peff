#ifndef _PHUL_BASE_PMR_H_
#define _PHUL_BASE_PMR_H_

#include "allocator.h"

namespace phul {
	class MemoryResource {
	public:
		PHUL_BASE_API ~MemoryResource();

		[[nodiscard]] virtual void *alloc(size_t size, size_t alignment) = 0;
		[[nodiscard]] virtual void release(void *ptr) = 0;
	};

	class DefaultMemoryResource final : public MemoryResource {
	private:
		StdAlloc _allocator;

	public:
		PHUL_BASE_API DefaultMemoryResource();
		PHUL_BASE_API ~DefaultMemoryResource();

		PHUL_BASE_API [[nodiscard]] virtual void *alloc(size_t size, size_t alignment) override;
		PHUL_BASE_API [[nodiscard]] virtual void release(void *ptr) override;
	};

	extern DefaultMemoryResource g_defaultMemoryResource;

	class MonotonicBufferMemoryResource final : public MemoryResource {
	private:
		size_t _szAllocated = 0;

	public:
		MemoryResource *upstream;
		void *buffer;
		size_t size;

		PHUL_BASE_API MonotonicBufferMemoryResource(void *buffer, size_t size);
		PHUL_BASE_API ~MonotonicBufferMemoryResource();

		PHUL_BASE_API [[nodiscard]] virtual void *alloc(size_t size, size_t alignment) override;
		PHUL_BASE_API [[nodiscard]] virtual void release(void *ptr) override;
	};

	class PmrAlloc {
	public:
		MemoryResource *memoryResource = &g_defaultMemoryResource;

		PHUL_BASE_API void *alloc(size_t size, size_t alignment);
		PHUL_BASE_API void release(void *ptr);
	};
}

#endif
