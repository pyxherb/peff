#ifndef _PEFF_ADVUTILS_BUFFER_ALLOC_H_
#define _PEFF_ADVUTILS_BUFFER_ALLOC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <peff/containers/tree.h>

namespace peff {
	class BufferAlloc : public Alloc {
	public:
		struct AllocDesc;

		struct AllocDescComparator {
			PEFF_FORCEINLINE bool operator()(const void *lhs, const void *rhs) {
				return lhs < rhs;
			}
		};

		struct AllocDesc : public RBTree<void *, AllocDescComparator>::Node {
			size_t size;
			size_t alignment;

			PEFF_FORCEINLINE AllocDesc(void *ptr) : Node(std::move(ptr)) {}
		};

		char *buffer;
		size_t bufferSize;
		RBTree<void *, AllocDescComparator> allocDescs;

		PEFF_ADVUTILS_API BufferAlloc(char *buffer, size_t bufferSize);

		PEFF_ADVUTILS_API virtual void onRefZero() noexcept override;

		PEFF_ADVUTILS_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_ADVUTILS_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_ADVUTILS_API virtual Alloc *getDefaultAlloc() const noexcept override;
	};

	extern BufferAlloc g_bufferAlloc;
	extern RcObjectPtr<BufferAlloc> g_bufferAllocKeeper;
}

#endif
