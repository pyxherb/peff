#include <cstdio>
#include <peff/advutils/buffer_alloc.h>
#include <peff/containers/dynarray.h>
#include <iostream>
#include <string>
#include <coroutine>

struct Coroutine {
	struct promise_type;

	using Handle = std::coroutine_handle<promise_type>;

	struct promise_type {
		int result;

		static Coroutine get_return_object_on_allocation_failure() noexcept {
			return Coroutine({});
		}

		Coroutine get_return_object() noexcept {
			return Coroutine(Handle::from_promise(*this));
		}

		std::suspend_always initial_suspend() noexcept {
			return {};
		}

		std::suspend_always final_suspend() noexcept {
			return {};
		}

		std::suspend_always yield_value(int value) noexcept {
			result = value;
			return {};
		}

		void return_value(int value) noexcept {
			result = value;
		}

		void unhandled_exception() { std::terminate(); }

		struct AllocatorInfo {
			peff::Alloc *allocator;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			size_t c;
#endif
		};

		static void *operator new(size_t size, peff::Alloc *allocator, int n) noexcept {
			char *p = (char *)allocator->alloc(size + sizeof(AllocatorInfo), sizeof(std::max_align_t));

			if (!p)
				return nullptr;

			AllocatorInfo allocator_info = {
				allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				,
				peff::g_rcobj_ptr_counter
#endif
			};

			memcpy(p + size, &allocator_info, sizeof(allocator_info));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			allocator->inc_ref(peff::g_rcobj_ptr_counter++);
#endif

			return p;
		}

		static void operator delete(void *p, size_t size) noexcept {
			AllocatorInfo allocator_info;

			memcpy(&allocator_info, (char *)p + size, sizeof(allocator_info));

			allocator_info.allocator->release(p, size, sizeof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			allocator_info.allocator->dec_ref(allocator_info.c);
#endif
		}
	};

	Handle coro_handle;

	Coroutine(Handle coro_handle) : coro_handle(coro_handle) {}
	~Coroutine() {
		if (coro_handle)
			coro_handle.destroy();
	}

	bool done() {
		return coro_handle.done();
	}

	int resume() {
		if (!coro_handle.done()) {
			coro_handle.resume();
			if (!coro_handle)
				return INT_MIN;
			return coro_handle.promise().result;
		}

		std::terminate();
	}
};

Coroutine coroutine(peff::Alloc *allocator, int n) {
	while (n) {
		co_yield n;

		--n;
	}

	co_return 0;
}

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Coroutine co = coroutine(peff::default_allocator(), 100);

	while (!co.done()) {
		printf("%d\n", co.resume());
	}

	return 0;
}
