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
		int returnValue;

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
			returnValue = value;
			return {};
		}

		void return_value(int value) noexcept {
			returnValue = value;
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

			AllocatorInfo allocatorInfo = {
				allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				,
				peff::g_rcObjectPtrCounter
#endif
			};

			memcpy(p + size, &allocatorInfo, sizeof(allocatorInfo));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			allocator->incRef(peff::g_rcObjectPtrCounter++);
#endif

			return p;
		}

		static void operator delete(void *p, size_t size) noexcept {
			AllocatorInfo allocatorInfo;

			memcpy(&allocatorInfo, (char *)p + size, sizeof(allocatorInfo));

			allocatorInfo.allocator->release(p, size, sizeof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
			allocatorInfo.allocator->decRef(allocatorInfo.c);
#endif
		}
	};

	Handle coroutineHandle;

	Coroutine(Handle coroutineHandle) : coroutineHandle(coroutineHandle) {}
	~Coroutine() {
		if (coroutineHandle)
			coroutineHandle.destroy();
	}

	bool done() {
		return coroutineHandle.done();
	}

	int resume() {
		if (!coroutineHandle.done()) {
			coroutineHandle.resume();
			if (!coroutineHandle)
				return INT_MIN;
			return coroutineHandle.promise().returnValue;
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

	Coroutine co = coroutine(peff::getDefaultAlloc(), 100);

	while (!co.done()) {
		printf("%d\n", co.resume());
	}

	return 0;
}
