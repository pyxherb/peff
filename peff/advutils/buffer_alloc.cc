#include "buffer_alloc.h"

using namespace peff;

BufferAlloc peff::g_bufferAlloc(nullptr, 0);
RcObjectPtr<BufferAlloc> peff::g_bufferAllocKeeper(&g_bufferAlloc);

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(char *buffer, size_t bufferSize) : buffer(buffer), bufferSize(bufferSize), allocDescs(&g_voidAlloc, AllocDescComparator()) {
}

PEFF_ADVUTILS_API void BufferAlloc::onRefZero() noexcept {
}

PEFF_ADVUTILS_API void *BufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	size_t off = 0, descSize = sizeof(AllocDesc), descOff;

	size_t actualAvailableSize = size + descSize;

	if (size_t alignedDiff = actualAvailableSize % alignment; alignedDiff) {
		actualAvailableSize += alignment - alignedDiff;
	}

	descOff = actualAvailableSize;
	if (size_t alignedDiff = descOff % alignof(AllocDesc); descOff) {
		descSize += alignof(AllocDesc) - alignedDiff;
	}

	size_t actualSize = descOff + sizeof(AllocDesc);

	while (off < (bufferSize - actualSize)) {
		if (size_t alignedDiff = ((uintptr_t)(buffer + off)) % alignment; alignedDiff) {
			off += alignment - alignedDiff;
			continue;
		}

		{
			AllocDesc *bottomDesc = (AllocDesc *)allocDescs.getMaxLteqNode(buffer + off);

			if (bottomDesc) {
				size_t bottomDescOff = (((char*)bottomDesc->descBase) - buffer);
				if (bottomDescOff + sizeof(AllocDesc) > off) {
					off = bottomDescOff + sizeof(AllocDesc);
					continue;
				}
			}
		}
		{
			AllocDesc *topDesc = (AllocDesc *)allocDescs.getMaxLteqNode(buffer + off + actualSize);

			if (topDesc) {
				size_t topDescOff = (((char*)topDesc->descBase) - buffer);
				if (topDescOff + sizeof(AllocDesc) > off) {
					off = topDescOff + sizeof(AllocDesc);
					continue;
				}
			}
		}
		goto succeeded;
	}

	return nullptr;

succeeded:
	char *ptr = buffer + off, *allocDescRawPtr = ptr + descOff;
	constructAt<AllocDesc>((AllocDesc*)allocDescRawPtr, (AllocDesc*)ptr);

	AllocDesc *allocDescPtr = (AllocDesc*)allocDescRawPtr;

	allocDescPtr->size = size;
	allocDescPtr->alignment = alignment;
	allocDescPtr->descBase = allocDescPtr;

	bool result = allocDescs.insert((RBTree<void*, AllocDescComparator>::Node*)allocDescRawPtr);
	assert(result);

	return ptr;
}

PEFF_ADVUTILS_API void BufferAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
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

PEFF_ADVUTILS_API Alloc *BufferAlloc::getDefaultAlloc() const noexcept {
	return g_voidAllocKeeper.get();
}
