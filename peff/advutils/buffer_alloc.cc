#include "buffer_alloc.h"

using namespace peff;

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(char *buffer, size_t bufferSize) : buffer(buffer), bufferSize(bufferSize), allocDescs(&g_voidAlloc, AllocDescComparator()) {
}

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(BufferAlloc &&rhs) noexcept : buffer(rhs.buffer), bufferSize(rhs.bufferSize), allocDescs(std::move(rhs.allocDescs)) {
	rhs.buffer = nullptr;
	rhs.bufferSize = 0;
}

PEFF_ADVUTILS_API BufferAlloc &BufferAlloc::operator=(BufferAlloc &&rhs) noexcept {
	buffer = rhs.buffer;
	bufferSize = rhs.bufferSize;
	allocDescs = std::move(rhs.allocDescs);

	rhs.buffer = nullptr;
	rhs.bufferSize = 0;

	return *this;
}

PEFF_ADVUTILS_API size_t BufferAlloc::decRef(size_t globalRc) noexcept {
	if (!--_refCount) {
		onRefZero();
		return 0;
	}
	return _refCount;
}

PEFF_ADVUTILS_API size_t BufferAlloc::incRef(size_t globalRc) noexcept {
	return ++_refCount;
}

PEFF_ADVUTILS_API void BufferAlloc::onRefZero() noexcept {
}

PEFF_ADVUTILS_API void *BufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	size_t off = 0, descOff;

	size_t actualSize = calcAllocSize(size, alignment, &descOff);

	while (off + actualSize <= bufferSize) {
		if (size_t alignedDiff = ((uintptr_t)(buffer + off)) % alignment; alignedDiff) {
			off += alignment - alignedDiff;
			continue;
		}

		{
			AllocDesc *bottomDesc = (AllocDesc *)allocDescs.getMaxLteqNode(buffer + off);

			if (bottomDesc) {
				size_t bottomDescOff = (((char *)bottomDesc->descBase) - buffer);
				if (bottomDescOff + sizeof(AllocDesc) > off) {
					off = bottomDescOff + sizeof(AllocDesc);
					continue;
				}
			}
		}
		{
			AllocDesc *topDesc = (AllocDesc *)allocDescs.getMaxLteqNode(buffer + off + actualSize);

			if (topDesc) {
				size_t topDescOff = (((char *)topDesc->descBase) - buffer);
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
	constructAt<AllocDesc>((AllocDesc *)allocDescRawPtr, (AllocDesc *)ptr);

	AllocDesc *allocDescPtr = (AllocDesc *)allocDescRawPtr;

	allocDescPtr->size = size;
	allocDescPtr->alignment = alignment;
	allocDescPtr->descBase = allocDescPtr;

	assert(((uintptr_t)ptr + size) <= ((uintptr_t)allocDescPtr));

	bool result = allocDescs.insert(allocDescPtr);
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

PEFF_ADVUTILS_API bool BufferAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	if (rhs->getTypeId() != getTypeId()) {
		return false;
	}

	const BufferAlloc *r = (const BufferAlloc *)rhs;

	if (buffer != r->buffer)
		return false;

	if (bufferSize != r->bufferSize)
		return false;

	return true;
}

PEFF_ADVUTILS_API UUID BufferAlloc::getTypeId() const noexcept {
	return PEFF_UUID(8c2a0e1b, 4d3a, 4e6f, 8a2c, 6b0e1d9f4c8);
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc::UpstreamedBufferAlloc(peff::BufferAlloc *bufferAlloc, peff::Alloc *upstream) : bufferAlloc(bufferAlloc), upstream(upstream) {
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc::UpstreamedBufferAlloc(UpstreamedBufferAlloc &&rhs) noexcept : bufferAlloc(std::move(rhs.bufferAlloc)), upstream(std::move(rhs.upstream)) {
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc &UpstreamedBufferAlloc::operator=(UpstreamedBufferAlloc &&rhs) noexcept {
	bufferAlloc = std::move(rhs.bufferAlloc);
	upstream = std::move(rhs.upstream);

	return *this;
}

PEFF_ADVUTILS_API size_t UpstreamedBufferAlloc::decRef(size_t globalRc) noexcept {
	if (!--_refCount) {
		onRefZero();
		return 0;
	}
	return _refCount;
}

PEFF_ADVUTILS_API size_t UpstreamedBufferAlloc::incRef(size_t globalRc) noexcept {
	return ++_refCount;
}

PEFF_ADVUTILS_API void UpstreamedBufferAlloc::onRefZero() noexcept {
}

PEFF_FORCEINLINE static size_t _calcMarkerPos(size_t size, size_t alignment) {
	if (size_t diff = size % alignment; diff) {
		return size + (alignment - diff);
	}
	return size;
}

PEFF_ADVUTILS_API void *UpstreamedBufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *ptr;

	size_t offMarker = _calcMarkerPos(size, alignof(size_t));
	if ((ptr = bufferAlloc->alloc(size + offMarker + sizeof(uintptr_t), alignment))) {
		Alloc **markerPtr = (Alloc **)(((char *)ptr) + offMarker);

		*markerPtr = bufferAlloc.get();
		return ptr;
	}

	if ((ptr = upstream->alloc(size + offMarker + sizeof(uintptr_t), alignment))) {
		Alloc **markerPtr = (Alloc **)(((char *)ptr) + offMarker);

		*markerPtr = upstream.get();
		return ptr;
	}

	return nullptr;
}

PEFF_ADVUTILS_API void UpstreamedBufferAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	size_t offMarker = _calcMarkerPos(size, alignof(size_t));
	Alloc **markerPtr = ((Alloc **)((char *)ptr + offMarker));
	Alloc *marker = *markerPtr;

	marker->release(ptr, size + offMarker + sizeof(uintptr_t), alignment);
}

PEFF_ADVUTILS_API bool UpstreamedBufferAlloc::isReplaceable(const Alloc *rhs) const noexcept {
	if (rhs->getTypeId() != getTypeId()) {
		return false;
	}

	const UpstreamedBufferAlloc *r = (const UpstreamedBufferAlloc *)rhs;

	if (bufferAlloc != r->bufferAlloc)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

PEFF_ADVUTILS_API UUID UpstreamedBufferAlloc::getTypeId() const noexcept {
	return PEFF_UUID(c3b9e6f0, 1a2f, 4c39, 1e6c, 3d4e1f2b3c82);
}
