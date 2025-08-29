#ifndef _PEFF_ADVUTILS_BUFFER_ALLOC_H_
#define _PEFF_ADVUTILS_BUFFER_ALLOC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <peff/containers/tree.h>

namespace peff {
	class BufferAlloc : public Alloc {
	protected:
		std::atomic_size_t _refCount = 0;

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
			AllocDesc *descBase;

			PEFF_FORCEINLINE AllocDesc(void *ptr) : Node(std::move(ptr)) {}
		};

		char *buffer;
		size_t bufferSize;
		RBTree<void *, AllocDescComparator> allocDescs;

		PEFF_ADVUTILS_API BufferAlloc(char *buffer, size_t bufferSize);
		PEFF_ADVUTILS_API BufferAlloc(BufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API BufferAlloc &operator=(BufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API virtual size_t incRef(size_t globalRc) noexcept override;
		PEFF_ADVUTILS_API virtual size_t decRef(size_t globalRc) noexcept override;
		PEFF_ADVUTILS_API virtual void onRefZero() noexcept;

		PEFF_ADVUTILS_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_ADVUTILS_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_ADVUTILS_API virtual bool isReplaceable(const Alloc *rhs) const noexcept override;

		PEFF_ADVUTILS_API virtual UUID getTypeId() const noexcept override;

		PEFF_FORCEINLINE static size_t calcAllocSize(size_t size, size_t alignment, size_t *descOffOut = nullptr) noexcept {
			size_t descSize = sizeof(AllocDesc);

			size_t actualAvailableSize = size + descSize;

			if (size_t alignedDiff = actualAvailableSize % alignment; alignedDiff) {
				actualAvailableSize += alignment - alignedDiff;
			}

			size_t descOff = actualAvailableSize;
			if (size_t alignedDiff = descOff % alignof(AllocDesc); alignedDiff) {
				descSize += alignof(AllocDesc) - alignedDiff;
			}

			if (descOffOut)
				*descOffOut = descOff;

			return descOff + sizeof(AllocDesc);
		}
	};

	extern BufferAlloc g_bufferAlloc;
	extern RcObjectPtr<BufferAlloc> g_bufferAllocKeeper;
}

#endif
