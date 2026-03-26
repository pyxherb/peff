#ifndef _PEFF_ADVUTILS_BUFFER_ALLOC_H_
#define _PEFF_ADVUTILS_BUFFER_ALLOC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <peff/containers/rbtree.h>

namespace peff {
	class BufferAlloc : public Alloc {
	protected:
		std::atomic_size_t _ref_count = 0;

	public:
		struct AllocDesc;

		struct AllocDescComparator {
			PEFF_FORCEINLINE int operator()(const void *lhs, const void *rhs) const {
				if (lhs < rhs)
					return -1;
				if (lhs > rhs)
					return 1;
				return 0;
			}
		};

		struct AllocDesc : public RBTree<void *, AllocDescComparator, true>::Node {
			size_t size;
			size_t alignment;
			AllocDesc *desc_base;

			PEFF_FORCEINLINE AllocDesc(void *ptr) : Node(std::move(ptr)) {}
		};

		char *buffer;
		size_t buffer_size;
		RBTree<void *, AllocDescComparator, true> alloc_descs;

		PEFF_ADVUTILS_API BufferAlloc(char *buffer, size_t buffer_size);
		PEFF_ADVUTILS_API BufferAlloc(BufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API BufferAlloc &operator=(BufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API virtual size_t inc_ref(size_t global_ref_count) noexcept override;
		PEFF_ADVUTILS_API virtual size_t dec_ref(size_t global_ref_count) noexcept override;
		PEFF_ADVUTILS_API virtual void on_ref_zero() noexcept;

		PEFF_ADVUTILS_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_ADVUTILS_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_ADVUTILS_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_ADVUTILS_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_ADVUTILS_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		PEFF_ADVUTILS_API virtual UUID type_identity() const noexcept override;

		PEFF_FORCEINLINE constexpr static size_t calc_alloc_size(size_t size, size_t alignment, size_t *desc_off_out = nullptr) noexcept {
			size_t user_data_size = size;

			if (size_t aligned_diff = user_data_size % alignment; aligned_diff) {
				user_data_size += alignment - aligned_diff;
			}

			size_t desc_off = user_data_size;
			if (size_t aligned_diff = desc_off % alignof(AllocDesc); aligned_diff) {
				desc_off += alignof(AllocDesc) - aligned_diff;
			}

			if (desc_off_out)
				*desc_off_out = desc_off;

			return desc_off + sizeof(AllocDesc);
		}
	};

	class UpstreamedBufferAlloc : public Alloc {
	protected:
		std::atomic_size_t _ref_count = 0;

	public:
		peff::RcObjectPtr<peff::BufferAlloc> buffer_alloc;
		peff::RcObjectPtr<peff::Alloc> upstream;

		PEFF_ADVUTILS_API UpstreamedBufferAlloc(peff::BufferAlloc *buffer_alloc, peff::Alloc *upstream);
		PEFF_ADVUTILS_API UpstreamedBufferAlloc(UpstreamedBufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API UpstreamedBufferAlloc &operator=(UpstreamedBufferAlloc &&rhs) noexcept;

		PEFF_ADVUTILS_API virtual size_t inc_ref(size_t global_ref_count) noexcept override;
		PEFF_ADVUTILS_API virtual size_t dec_ref(size_t global_ref_count) noexcept override;
		PEFF_ADVUTILS_API virtual void on_ref_zero() noexcept;

		PEFF_ADVUTILS_API virtual void *alloc(size_t size, size_t alignment = 0) noexcept override;
		PEFF_ADVUTILS_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_ADVUTILS_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		PEFF_ADVUTILS_API virtual void release(void *ptr, size_t size, size_t alignment) noexcept override;

		PEFF_ADVUTILS_API virtual bool is_replaceable(const Alloc *rhs) const noexcept override;

		PEFF_ADVUTILS_API virtual UUID type_identity() const noexcept override;
	};
}

#endif
