#include <cstdio>
#include <peff/containers/set.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <peff/containers/hashset.h>
#include <peff/containers/hashmap.h>
#include <peff/containers/map.h>
#include <peff/containers/bitarray.h>
#include <peff/advutils/shared_ptr.h>
#include <peff/advutils/buffer_alloc.h>
#include <iostream>
#include <string>

struct SomethingUncopyable {
	peff::String s;
};

class RcObj;

char g_buffer[1048576];
peff::RcObjectPtr<RcObj> test;

struct Test {
	uint8_t test[1024];

	virtual void testA() {
		puts("testA");
	}
};

struct Test2 : public peff::RcObject, public peff::SharedFromThis<Test2> {
	uint8_t test2[1024];

	virtual void testB() {
		puts("testB");
	}
};

class RcObj : public Test, public Test2 {
public:
	const char *name;
	RcObj(const char *name): name(name) {
		memset(test, 0xcc, sizeof(test));
		memset(test2, 0xdd, sizeof(test2));
	}
	virtual ~RcObj() {
		for (size_t i = 0; i < std::size(test); ++i) {
			assert(test[i] == 0xcc);
		}
		for (size_t i = 0; i < std::size(test); ++i) {
			assert(test2[i] == 0xdd);
		}
		printf("Destructed %s\n", name);
	}
	virtual void onRefZero() noexcept {
		delete this;
	}
};

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	peff::BufferAlloc globalBufferAlloc(g_buffer, sizeof(g_buffer));

	peff::String s(&globalBufferAlloc);
	s.resize(3);
	memcpy(s.data(), "123", 3);
	assert(((std::string_view)s) == "123");

	peff::DynArray<SomethingUncopyable> a(&globalBufferAlloc);

	peff::SharedPtr<RcObj> outerSharedPtr;

	{
		peff::SharedPtr<RcObj> sharedPtr = peff::makeShared<RcObj>(peff::getDefaultAlloc(), "SharedPtr");
		peff::SharedPtr<Test2> test2Ptr = sharedPtr.castTo<Test2>();

		outerSharedPtr = sharedPtr;

		auto test2PtrLocked = peff::WeakPtr<Test2>(test2Ptr).lock();
		auto test2PtrFromThis = test2Ptr->sharedFromThis();

		peff::Set<int> map(&globalBufferAlloc);
		peff::RcObjectPtr<RcObj> strongRef;
		strongRef = new RcObj("StrongRef");

		peff::RcObject *weak = strongRef.get();

		test = strongRef;

		strongRef->testA();
		strongRef->testB();

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Inserting: %d\n", j);
			if (!map.insert(std::move(j)))
				throw std::bad_alloc();

			map.verify();
		}

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Removing: %d\n", j);

			map.remove(j);

			auto k = map.begin();
			while (k != map.end()) {
				printf("%d\n", *(k++));
			}

			// map.dump(std::cout);

			map.verify();
		}
		peff::Set<int> map2(&globalBufferAlloc);
		if (!peff::copyAssign(map2, map))
			throw std::bad_alloc();

		auto k = map2.begin();
		while (k != map2.end()) {
			printf("%d\n", *(k++));
		}
	}
	test.reset();
	outerSharedPtr.reset();
	{
		peff::HashSet<int> map(&globalBufferAlloc);

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Inserting: %d\n", j);
			if (!map.insert(std::move(j)))
				throw std::bad_alloc();

			auto k = map.begin();
			while (k != map.end()) {
				printf("%d\n", *(k++));
			}
		}

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Removing: %d\n", j);

			map.remove(j);

			auto k = map.begin();
			while (k != map.end()) {
				printf("%d\n", *(k++));
			}

			// map.dump(std::cout);
		}
		/*
		peff::HashSet<int> map2(&myStdAlloc);
		if (!peff::copyAssign(map2, map))
			throw std::bad_alloc();

		auto k = map2.begin();
		while (k != map2.end()) {
			printf("%d\n", *(k++));
		}*/
	}
	{
		peff::Map<int, peff::String> map(&globalBufferAlloc);

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;

			peff::String s(&globalBufferAlloc);
			{
				std::string stdString = std::to_string(j);
				if (!s.resize(stdString.size()))
					throw std::bad_alloc();
				memcpy(s.data(), stdString.data(), stdString.size());
			}

			std::string_view sv = (std::string_view)s;
			std::cout << "Dumping string: " << sv << std::endl;

			printf("Inserting: %d\n", j);
			if (!map.insert(std::move(j), std::move(s)))
				throw std::bad_alloc();
		}

		peff::Map<int, peff::String> map2(peff::getDefaultAlloc());
		if (!peff::copy(map2, map)) {
			throw std::bad_alloc();
		}
	}

	{
		peff::DynArray<int> arr(&globalBufferAlloc);
		peff::String str(&globalBufferAlloc);

		for (int i = 0; i < 32; i++) {
			int tmp = i;
			if (!arr.pushFront(std::move(tmp)))
				throw std::bad_alloc();
			if (!str.insert(0, 'a' + i))
				throw std::bad_alloc();

			for (size_t i = 0; i < arr.size(); ++i) {
				printf("%d ", arr.at(i));
			}

			puts("");

			for (size_t i = 0; i < str.size(); ++i) {
				printf("%c ", str.at(i));
			}

			puts("");
		}

		arr.extractRange(10, 20);
		if (!arr.eraseRange(0, 7))
			throw std::bad_alloc();

		str.extractRange(10, 20);
		if (!str.eraseRange(0, 7))
			throw std::bad_alloc();

		for (size_t i = 0; i < arr.size(); ++i) {
			printf("%d ", arr.at(i));
		}

		puts("");

		for (size_t i = 0; i < str.size(); ++i) {
			printf("%c ", str.at(i));
		}

		puts("");
	}

	{
		peff::BitArray bitArr(&globalBufferAlloc);

		bitArr.resize(64);

		bitArr.fillSet(0, 64);
		bitArr.fillClear(0, 48);

		for (size_t i = 0; i < bitArr.bitSize(); ++i) {
			printf("%s", bitArr.getBit(i) ? "1" : "0");
		}

		puts("");
	}

	/*{
		char buffer[32768];
		peff::BufferAlloc bufferAlloc(buffer, sizeof(buffer));

		for (size_t i = 1; i < 1024; ++i) {
			void *p1 = bufferAlloc.alloc(i, i);
			printf("Allocated: %p\n", p1);
			void *p2 = bufferAlloc.alloc(i * 2, i * 2);
			printf("Allocated: %p\n", p2);
			memset(p1, 0, i);
			memset(p2, 0, i * 2);
			bufferAlloc.release(p1, i, i);
			bufferAlloc.release(p2, i * 2, i * 2);
			void *p3 = bufferAlloc.alloc(i * 3, i * 3);
			printf("Allocated: %p\n", p3);
			memset(p3, 0, i * 3);
			bufferAlloc.release(p3, i * 3, i * 3);
		}
	}*/

	return 0;
}
