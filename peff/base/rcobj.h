#ifndef _PEFF_BASE_RCOBJ_H_
#define _PEFF_BASE_RCOBJ_H_

#include "basedefs.h"
#include <mutex>
#include <cstddef>
#include <atomic>
#include <cassert>
#include <type_traits>

namespace peff {
	template <typename T, typename V = void, typename W = void>
	struct IsRcObject : std::false_type {
	};

	template <typename T>
	struct IsRcObject<T, std::void_t<decltype(std::declval<T>().inc_ref((size_t)0))>, std::void_t<decltype(std::declval<T>().dec_ref((size_t)0))>> : std::true_type {
	};

#if PEFF_ENABLE_RCOBJ_DEBUGGING
	class RcObjectPtrCounterResetEventListener {
	private:
		RcObjectPtrCounterResetEventListener *_prev = nullptr, *_next = nullptr;

		friend PEFF_BASE_API void register_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener);
		friend PEFF_BASE_API void unregister_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener);

	public:
		PEFF_BASE_API RcObjectPtrCounterResetEventListener();
		PEFF_BASE_API virtual ~RcObjectPtrCounterResetEventListener();

		virtual void on_trigger() = 0;
	};

	PEFF_BASE_API extern RcObjectPtrCounterResetEventListener *g_rcobj_ptr_counter_reset_event_listener;
	PEFF_BASE_API extern std::atomic_size_t g_rcobj_ptr_counter;

	PEFF_BASE_API size_t acquire_global_rcobj_ptr_counter();
	PEFF_BASE_API void register_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener);
	PEFF_BASE_API void unregister_rcobj_ptr_counter_reset_event_listener(RcObjectPtrCounterResetEventListener *listener);
#endif

#if __cplusplus >= 202002L
	template <typename T>
	concept RcObjectTrait = requires(T * rcobj) {
		rcobj->inc_ref(0);
		rcobj->dec_ref(0);
	};
#endif

	template <typename T>
	class RcObjectPtr {
	public:
#if PEFF_ENABLE_RCOBJ_DEBUGGING
		size_t _counter = SIZE_MAX;
#endif

	private:
		using ThisType = RcObjectPtr<T>;

		T *_ptr = nullptr;

		PEFF_FORCEINLINE void _set_and_inc_ref(T *_ptr)
			PEFF_REQUIRES_CONCEPT(RcObjectTrait<T>) {
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			_counter = acquire_global_rcobj_ptr_counter();
			_ptr->inc_ref(_counter);
#else
			_ptr->inc_ref(SIZE_MAX);
#endif
			this->_ptr = _ptr;
		}

	public:
		PEFF_FORCEINLINE void reset() noexcept
			PEFF_REQUIRES_CONCEPT(RcObjectTrait<T>) {
			if (_ptr) {
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				_ptr->dec_ref(_counter);
#else
				_ptr->dec_ref(SIZE_MAX);
#endif
			}
			_ptr = nullptr;
		}

		PEFF_FORCEINLINE RcObjectPtr() : _ptr(nullptr) {
		}
		PEFF_FORCEINLINE RcObjectPtr(T *ptr) noexcept : _ptr(nullptr) {
			if (ptr) {
				_set_and_inc_ref(ptr);
			}
		}
		PEFF_FORCEINLINE RcObjectPtr(const ThisType &other) noexcept : _ptr(nullptr) {
			if (other._ptr) {
				_set_and_inc_ref(other._ptr);
			}
		}
		PEFF_FORCEINLINE RcObjectPtr(ThisType &&other) noexcept {
			_ptr = other._ptr;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			_counter = other._counter;
#endif
			other._ptr = nullptr;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			other._counter = SIZE_MAX;
#endif
		}
		PEFF_FORCEINLINE ~RcObjectPtr() {
			reset();
		}

		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(T *_ptr) noexcept {
			reset();
			if (_ptr) {
				_set_and_inc_ref(_ptr);
			}
			return *this;
		}
		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(const RcObjectPtr<T> &other) noexcept {
			reset();
			_set_and_inc_ref(other._ptr);
			return *this;
		}
		PEFF_FORCEINLINE RcObjectPtr<T> &operator=(RcObjectPtr<T> &&other) noexcept {
			reset();

			_ptr = other._ptr;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			_counter = other._counter;
#endif

			other._ptr = nullptr;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			other._counter = SIZE_MAX;
#endif

			return *this;
		}

		PEFF_FORCEINLINE T *get() const noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T *&get_ref() noexcept {
			reset();
			return _ptr;
		}
		PEFF_FORCEINLINE T *&get_ref_without_release() noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T *const &get_ref_without_release() const noexcept {
			return _ptr;
		}
		PEFF_FORCEINLINE T **get_addr() noexcept {
			reset();
			return &_ptr;
		}
		PEFF_FORCEINLINE T **get_addr_without_release() noexcept {
			return &_ptr;
		}
		PEFF_FORCEINLINE T *const *get_addr_without_release() const noexcept {
			return &_ptr;
		}
		PEFF_FORCEINLINE T *operator->() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
			return _ptr < rhs._ptr;
		}

		PEFF_FORCEINLINE operator bool() const noexcept {
			return _ptr;
		}

		PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
			return _ptr == rhs._ptr;
		}

		PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
			return _ptr != rhs._ptr;
		}
	};
}

#endif
