#ifndef _PEFF_BASE_PMR_H_
#define _PEFF_BASE_PMR_H_

#include "allocator.h"

namespace peff {
	class MemoryResource {
	public:
		PEFF_BASE_API ~MemoryResource();

		[[nodiscard]] virtual void *alloc(size_t size, size_t alignment) = 0;
		virtual void release(void *ptr) = 0;
	};

	class DefaultMemoryResource final : public MemoryResource {
	private:
		StdAlloc _allocator;

	public:
		PEFF_BASE_API DefaultMemoryResource();
		PEFF_BASE_API ~DefaultMemoryResource();

		PEFF_BASE_API [[nodiscard]] virtual void *alloc(size_t size, size_t alignment) override;
		PEFF_BASE_API virtual void release(void *ptr) override;
	};

	extern DefaultMemoryResource g_defaultMemoryResource;

	class MonotonicBufferMemoryResource final : public MemoryResource {
	private:
		size_t _szAllocated = 0;

	public:
		MemoryResource *upstream;
		void *buffer;
		size_t size;

		PEFF_BASE_API MonotonicBufferMemoryResource(void *buffer, size_t size);
		PEFF_BASE_API ~MonotonicBufferMemoryResource();

		PEFF_BASE_API [[nodiscard]] virtual void *alloc(size_t size, size_t alignment) override;
		PEFF_BASE_API virtual void release(void *ptr) override;
	};

	class PmrAlloc {
	public:
		MemoryResource *memoryResource = &g_defaultMemoryResource;

		PEFF_BASE_API void *alloc(size_t size, size_t alignment);
		PEFF_BASE_API void release(void *ptr);
	};
}

#endif
