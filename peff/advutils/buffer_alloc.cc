#include "buffer_alloc.h"

using namespace peff;

BufferAlloc peff::g_bufferAlloc(nullptr, 0);
RcObjectPtr<BufferAlloc> peff::g_bufferAllocKeeper(&g_bufferAlloc);

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(char *buffer, size_t bufferSize) : buffer(buffer), bufferSize(bufferSize), allocDescs(&g_voidAlloc) {
}

PEFF_BASE_API void BufferAlloc::onRefZero() noexcept {
}

PEFF_BASE_API void *BufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	size_t off = 0, descSize = sizeof(AllocDesc);

	if (size_t alignedDiff = descSize % alignment; descSize) {
		descSize += alignment - alignedDiff;
	}

	size_t actualSize = size + descSize;

	if (size_t alignedDiff = actualSize % alignment; alignedDiff) {
		actualSize += alignment - alignedDiff;
	}

	while (off < bufferSize) {
		if (size_t alignedDiff = ((uintptr_t)(buffer + off)) % alignment; alignedDiff) {
			off += alignment - alignedDiff;
			continue;
		}

		{
			AllocDesc queryDesc((AllocDesc *)buffer + off);

			AllocDesc *bottomDesc = (AllocDesc *)allocDescs.getMaxLteqNode(&queryDesc);

			if (bottomDesc) {
				size_t bottomDescOff = (((char *)bottomDesc) - buffer);
				if (bottomDescOff + bottomDesc->size >= off) {
					off = (((char *)bottomDesc->value) - buffer) + bottomDesc->size;
					continue;
				}
			}
		}
		{
			AllocDesc queryDesc((AllocDesc *)buffer + off + actualSize);

			AllocDesc *topDesc = (AllocDesc *)allocDescs.getMaxLteqNode(&queryDesc);

			if (topDesc) {
				size_t topDescOff = (((char *)topDesc) - buffer);
				if (topDescOff + topDesc->size >= off) {
					off = (((char *)topDesc->value) - buffer) + topDesc->size;
					continue;
				}
			}
		}
		goto succeeded;
	}

	return nullptr;

succeeded:
	char *ptr = buffer + off, *availablePtr = ptr + descSize;
	constructAt<AllocDesc>((AllocDesc*)ptr, (AllocDesc*)availablePtr);

	AllocDesc *allocDescPtr = (AllocDesc*)ptr;

	allocDescPtr->size = size;
	allocDescPtr->alignment = alignment;

	bool result = allocDescs.insert((RBTree<void*, AllocDescComparator>::Node*)ptr);
	assert(result);

	return availablePtr;
}

PEFF_BASE_API void BufferAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	auto resultDesc = allocDescs.get(ptr);
	if (!resultDesc)
		std::terminate();

	AllocDesc *desc = (AllocDesc *)resultDesc;

	if (desc->size != size)
		std::terminate();
	if (desc->alignment != alignment)
		std::terminate();

	allocDescs.remove(desc, false);
}

PEFF_BASE_API Alloc *BufferAlloc::getDefaultAlloc() const noexcept {
	return g_voidAllocKeeper.get();
}
