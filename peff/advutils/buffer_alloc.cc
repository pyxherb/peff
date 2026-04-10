#include "buffer_alloc.h"

using namespace peff;

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(char *buffer, size_t buffer_size) : buffer(buffer), buffer_size(buffer_size), alloc_descs(&g_void_alloc, AllocDescComparator()) {
}

PEFF_ADVUTILS_API BufferAlloc::BufferAlloc(BufferAlloc &&rhs) noexcept : buffer(rhs.buffer), buffer_size(rhs.buffer_size), alloc_descs(std::move(rhs.alloc_descs)) {
	rhs.buffer = nullptr;
	rhs.buffer_size = 0;
}

PEFF_ADVUTILS_API BufferAlloc &BufferAlloc::operator=(BufferAlloc &&rhs) noexcept {
	buffer = rhs.buffer;
	buffer_size = rhs.buffer_size;
	alloc_descs = std::move(rhs.alloc_descs);

	rhs.buffer = nullptr;
	rhs.buffer_size = 0;

	return *this;
}

PEFF_ADVUTILS_API size_t BufferAlloc::dec_ref(size_t global_ref_count) noexcept {
	if (!--_ref_count) {
		on_ref_zero();
		return 0;
	}
	return _ref_count;
}

PEFF_ADVUTILS_API size_t BufferAlloc::inc_ref(size_t global_ref_count) noexcept {
	return ++_ref_count;
}

PEFF_ADVUTILS_API void BufferAlloc::on_ref_zero() noexcept {
}

PEFF_ADVUTILS_API void *BufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	size_t off = 0, desc_off;

	size_t actual_size = calc_alloc_size(size, alignment, &desc_off);

	while (off + actual_size <= buffer_size) {
		if (size_t aligned_diff = ((uintptr_t)(buffer + off)) % alignment; aligned_diff) {
			off += alignment - aligned_diff;
			continue;
		}

		{
			AllocDesc *bottom_desc = (AllocDesc *)alloc_descs.get_max_lteq(buffer + off);

			if (bottom_desc) {
				size_t bottom_desc_off = (((char *)bottom_desc->desc_base) - buffer);
				if (bottom_desc_off + sizeof(AllocDesc) > off) {
					off = bottom_desc_off + sizeof(AllocDesc);
					continue;
				}
			}
		}
		{
			AllocDesc *top_desc = (AllocDesc *)alloc_descs.get_max_lteq(buffer + off + actual_size);

			if (top_desc) {
				size_t top_desc_off = (((char *)top_desc->desc_base) - buffer);
				if (top_desc_off + sizeof(AllocDesc) > off) {
					off = top_desc_off + sizeof(AllocDesc);
					continue;
				}
			}
		}
		goto succeeded;
	}

	return nullptr;

succeeded:
	char *ptr = buffer + off, *alloc_desc_raw_ptr = ptr + desc_off;
	peff::construct_at<AllocDesc>((AllocDesc *)alloc_desc_raw_ptr, ptr);

	AllocDesc *alloc_desc_ptr = (AllocDesc *)alloc_desc_raw_ptr;

	alloc_desc_ptr->size = size;
	alloc_desc_ptr->alignment = alignment;
	alloc_desc_ptr->desc_base = alloc_desc_ptr;

	assert(((uintptr_t)ptr + size) <= ((uintptr_t)alloc_desc_ptr));

	bool result = alloc_descs.insert(alloc_desc_ptr);
	assert(result);

	return ptr;
}

PEFF_ADVUTILS_API void *BufferAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	AllocDesc *old_desc = (AllocDesc *)alloc_descs.get(ptr);
	assert(old_desc);

	assert(size == old_desc->size);
	assert(alignment == old_desc->alignment);

	size_t off = 0, desc_off;

	size_t actual_size = calc_alloc_size(new_size, new_alignment, &desc_off);

	alloc_descs.remove(old_desc, false);

	while (off + actual_size <= buffer_size) {
		if (size_t aligned_diff = ((uintptr_t)(buffer + off)) % new_alignment; aligned_diff) {
			off += new_alignment - aligned_diff;
			continue;
		}

		{
			AllocDesc *bottom_desc = (AllocDesc *)alloc_descs.get_max_lteq(buffer + off);

			if (bottom_desc) {
				size_t bottom_desc_off = (((char *)bottom_desc->desc_base) - buffer);
				if (bottom_desc_off + sizeof(AllocDesc) > off) {
					off = bottom_desc_off + sizeof(AllocDesc);
					continue;
				}
			}
		}
		{
			AllocDesc *top_desc = (AllocDesc *)alloc_descs.get_max_lteq(buffer + off + actual_size);

			if (top_desc) {
				size_t top_desc_off = (((char *)top_desc->desc_base) - buffer);
				if (top_desc_off + sizeof(AllocDesc) > off) {
					off = top_desc_off + sizeof(AllocDesc);
					continue;
				}
			}
		}
		goto succeeded;
	}

	{
		bool restoration_result = alloc_descs.insert(old_desc);
		assert(restoration_result);
	}

	return nullptr;

succeeded:
	std::destroy_at<AllocDesc>(old_desc);
	char *p = buffer + off, *alloc_desc_raw_ptr = p + desc_off;

	memmove(p, ptr, size);

	peff::construct_at<AllocDesc>((AllocDesc *)alloc_desc_raw_ptr, p);

	AllocDesc *alloc_desc_ptr = (AllocDesc *)alloc_desc_raw_ptr;

	alloc_desc_ptr->size = new_size;
	alloc_desc_ptr->alignment = new_alignment;
	alloc_desc_ptr->desc_base = alloc_desc_ptr;

	bool result = alloc_descs.insert(alloc_desc_ptr);
	assert(result);

	return p;
}

PEFF_ADVUTILS_API void *BufferAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	AllocDesc *old_desc = (AllocDesc *)alloc_descs.get(ptr);
	assert(old_desc);

	assert(size == old_desc->size);
	assert(alignment == old_desc->alignment);

	size_t desc_off;

	size_t actual_size = calc_alloc_size(new_size, new_alignment, &desc_off);

	peff::ScopeGuard sg([this, old_desc]() noexcept {
		bool restoration_result = alloc_descs.insert(old_desc);
		assert(restoration_result);
	});
	alloc_descs.remove(old_desc, false);

	if (new_alignment < alignment)
		return nullptr;

	{
		AllocDesc *top_desc = (AllocDesc *)alloc_descs.get_max_lteq(((char *)ptr) + actual_size);

		if (top_desc) {
			if ((char *)top_desc->desc_base >= ((char *)ptr) + actual_size)
				return nullptr;
		}
	}

	std::destroy_at<AllocDesc>((AllocDesc*)old_desc);
	peff::construct_at<AllocDesc>((AllocDesc *)((char*)ptr) + desc_off, ptr);

	AllocDesc *alloc_desc_ptr = (AllocDesc *)((char*)ptr) + desc_off;

	alloc_desc_ptr->size = new_size;
	alloc_desc_ptr->alignment = new_alignment;
	alloc_desc_ptr->desc_base = alloc_desc_ptr;

	bool result = alloc_descs.insert(alloc_desc_ptr);
	assert(result);

	return ptr;
}

PEFF_ADVUTILS_API void BufferAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	auto result_desc = alloc_descs.get(ptr);
	if (!result_desc)
		std::terminate();

	AllocDesc *desc = (AllocDesc *)result_desc;

	if (desc->size != size)
		std::terminate();
	if (desc->alignment != alignment)
		std::terminate();

	alloc_descs.remove(desc, false);
}

PEFF_ADVUTILS_API bool BufferAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	if (rhs->type_identity() != type_identity()) {
		return false;
	}

	const BufferAlloc *r = (const BufferAlloc *)rhs;

	if (buffer != r->buffer)
		return false;

	if (buffer_size != r->buffer_size)
		return false;

	return true;
}

PEFF_ADVUTILS_API UUID BufferAlloc::type_identity() const noexcept {
	return PEFF_UUID(8c2a0e1b, 4d3a, 4e6f, 8a2c, 6b0e1d9f4c8);
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc::UpstreamedBufferAlloc(peff::BufferAlloc *buffer_alloc, peff::Alloc *upstream) : buffer_alloc(buffer_alloc), upstream(upstream) {
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc::UpstreamedBufferAlloc(UpstreamedBufferAlloc &&rhs) noexcept : buffer_alloc(std::move(rhs.buffer_alloc)), upstream(std::move(rhs.upstream)) {
}

PEFF_ADVUTILS_API UpstreamedBufferAlloc &UpstreamedBufferAlloc::operator=(UpstreamedBufferAlloc &&rhs) noexcept {
	buffer_alloc = std::move(rhs.buffer_alloc);
	upstream = std::move(rhs.upstream);

	return *this;
}

PEFF_ADVUTILS_API size_t UpstreamedBufferAlloc::dec_ref(size_t global_ref_count) noexcept {
	if (!--_ref_count) {
		on_ref_zero();
		return 0;
	}
	return _ref_count;
}

PEFF_ADVUTILS_API size_t UpstreamedBufferAlloc::inc_ref(size_t global_ref_count) noexcept {
	return ++_ref_count;
}

PEFF_ADVUTILS_API void UpstreamedBufferAlloc::on_ref_zero() noexcept {
}

PEFF_FORCEINLINE static size_t _calc_marker_pos(size_t size, size_t alignment) {
	if (size_t diff = size % alignment; diff) {
		return size + (alignment - diff);
	}
	return size;
}

PEFF_ADVUTILS_API void *UpstreamedBufferAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *ptr;

	size_t off_marker = _calc_marker_pos(size, alignof(uintptr_t));
	if ((ptr = buffer_alloc->alloc(size + off_marker + sizeof(uintptr_t), alignment))) {
		Alloc **marker_ptr = (Alloc **)(((char *)ptr) + off_marker);

		*marker_ptr = buffer_alloc.get();
		return ptr;
	}

	if ((ptr = upstream->alloc(size + off_marker + sizeof(uintptr_t), alignment))) {
		Alloc **marker_ptr = (Alloc **)(((char *)ptr) + off_marker);

		*marker_ptr = upstream.get();
		return ptr;
	}

	return nullptr;
}

PEFF_ADVUTILS_API void *UpstreamedBufferAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	size_t off_marker = _calc_marker_pos(size, alignof(uintptr_t));
	Alloc **marker_ptr = ((Alloc **)((char *)ptr + off_marker));
	Alloc *marker = *marker_ptr;

	void *p;

	if (marker == buffer_alloc.get()) {
		size_t new_off_marker = _calc_marker_pos(new_size, alignof(size_t));

		if ((p = buffer_alloc->realloc(ptr, size + off_marker + sizeof(uintptr_t), alignment, new_size + new_off_marker + sizeof(uintptr_t), new_alignment))) {
			memmove(p, ptr, size);
			;

			Alloc **new_marker_ptr = (Alloc **)(((char *)p) + new_off_marker);

			*new_marker_ptr = buffer_alloc.get();
			return p;
		}
	}

	if (!(p = upstream->alloc(new_size + off_marker + sizeof(uintptr_t), new_alignment))) {
		return nullptr;
	}

	memmove(p, ptr, size);

	size_t new_off_marker = _calc_marker_pos(new_size, alignof(size_t));
	{
		Alloc **new_marker_ptr = (Alloc **)(((char *)p) + new_off_marker);

		*new_marker_ptr = upstream.get();
	}

	marker->release(ptr, size + off_marker + sizeof(uintptr_t), alignment);

	return p;
}

PEFF_ADVUTILS_API void *UpstreamedBufferAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	size_t off_marker = _calc_marker_pos(size, alignof(uintptr_t));
	Alloc **marker_ptr = ((Alloc **)((char *)ptr + off_marker));
	Alloc *marker = *marker_ptr;

	void *p;

	if (marker == buffer_alloc.get()) {
		size_t new_off_marker = _calc_marker_pos(new_size, alignof(size_t));

		if ((p = buffer_alloc->realloc_in_place(ptr, size + off_marker + sizeof(uintptr_t), alignment, new_size + new_off_marker + sizeof(uintptr_t), new_alignment))) {
			memmove(p, ptr, size);
			;

			Alloc **new_marker_ptr = (Alloc **)(((char *)p) + new_off_marker);

			*new_marker_ptr = buffer_alloc.get();
			return p;
		}
	}

	if (!(p = upstream->alloc(new_size + off_marker + sizeof(uintptr_t), new_alignment))) {
		return nullptr;
	}

	memmove(p, ptr, size);

	size_t new_off_marker = _calc_marker_pos(new_size, alignof(size_t));
	{
		Alloc **new_marker_ptr = (Alloc **)(((char *)p) + new_off_marker);

		*new_marker_ptr = upstream.get();
	}

	marker->release(ptr, size + off_marker + sizeof(uintptr_t), alignment);

	return p;
}

PEFF_ADVUTILS_API void UpstreamedBufferAlloc::release(void *ptr, size_t size, size_t alignment) noexcept {
	size_t off_marker = _calc_marker_pos(size, alignof(uintptr_t));
	Alloc **marker_ptr = ((Alloc **)((char *)ptr + off_marker));
	Alloc *marker = *marker_ptr;

	marker->release(ptr, size + off_marker + sizeof(uintptr_t), alignment);
}

PEFF_ADVUTILS_API bool UpstreamedBufferAlloc::is_replaceable(const Alloc *rhs) const noexcept {
	if (rhs->type_identity() != type_identity()) {
		return false;
	}

	const UpstreamedBufferAlloc *r = (const UpstreamedBufferAlloc *)rhs;

	if (buffer_alloc != r->buffer_alloc)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

PEFF_ADVUTILS_API UUID UpstreamedBufferAlloc::type_identity() const noexcept {
	return PEFF_UUID(c3b9e6f0, 1a2f, 4c39, 1e6c, 3d4e1f2b3c82);
}
