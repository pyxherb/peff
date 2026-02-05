#include <cstdio>
#include <peff/utils/byteord.h>
#include <peff/containers/set.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <peff/containers/hashset.h>
#include <peff/containers/hashmap.h>
#include <peff/containers/radix_tree.h>
#include <peff/containers/map.h>
#include <peff/containers/bitarray.h>
#include <peff/containers/binary_heap.h>
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

struct Test2 : public peff::SharedFromThis<Test2> {
	std::atomic_size_t ref = 0;

	virtual void onRefZero() noexcept = 0;

	size_t incRef(size_t globalRc) {
		return ++ref;
	}

	size_t decRef(size_t globalRc) {
		if (!--ref) {
			onRefZero();
			return 0;
		}
		return ref;
	}

	uint8_t test2[1024];

	virtual void testB() {
		puts("testB");
	}
};

class RcObj : public Test, public Test2 {
public:
	const char *name;
	RcObj(const char *name) : name(name) {
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

struct B {
	char *p;
	B() {
		p = new char[128];
	}
	~B() {
		if (p)
			delete[] p;
	}
	B(B &&rhs) : p(rhs.p) {
		rhs.p = nullptr;
	}
};

template <typename T>
struct FallibleComparator {
	peff::Option<bool> operator()(const T &lhs, const T &rhs) const {
		return {};
	}
};

template <typename T>
struct FallibleHasher {
	peff::Option<size_t> operator()(const T &lhs) const {
		return {};
	}
};

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	peff::String s(&peff::g_stdAlloc);
	s.resize(3);
	memcpy(s.data(), "123", 3);
	assert(((std::string_view)s) == "123");

	peff::DynArray<SomethingUncopyable> a(&peff::g_stdAlloc);

	peff::SharedPtr<RcObj> outerSharedPtr;

	{
		peff::FallibleHashSet<int, FallibleComparator<int>, FallibleHasher<int>> test(&peff::g_stdAlloc);

		for (int i = 0; i < 1024; ++i) {
			if (test.insert(+i)) {
				if (test.size() != 1) {
					std::terminate();
				}
			}
		}
	}
	{
		peff::SharedPtr<RcObj> sharedPtr = peff::makeShared<RcObj>(peff::getDefaultAlloc(), "SharedPtr");
		peff::SharedPtr<Test2> test2Ptr = sharedPtr.castTo<Test2>();

		outerSharedPtr = sharedPtr;

		auto test2PtrLocked = peff::WeakPtr<Test2>(test2Ptr).lock();
		auto test2PtrFromThis = test2Ptr->sharedFromThis();

		peff::Set<int> map(&peff::g_stdAlloc);
		peff::RcObjectPtr<RcObj> strongRef;
		strongRef = new RcObj("StrongRef");

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
	}
	test.reset();
	outerSharedPtr.reset();
	{
		peff::HashSet<int> map(&peff::g_stdAlloc);

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
		peff::Map<int, peff::String> map(&peff::g_stdAlloc);

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;

			peff::String s(&peff::g_stdAlloc);
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
	}

	{
		peff::BitArray bitArr(&peff::g_stdAlloc);

		bitArr.resizeUninitialized(64);

		bitArr.fillSet(0, 64);
		bitArr.fillClear(0, 48);

		for (size_t i = 0; i < bitArr.bitSize(); ++i) {
			printf("%s", bitArr.getBit(i) ? "1" : "0");
		}

		puts("");
	}

	{
		peff::DynArray<int> arr(&peff::g_stdAlloc);
		peff::String str(&peff::g_stdAlloc);

		for (int i = 0; i < 32; i++) {
			int tmp = i;
			if (!arr.insert(0, std::move(tmp)))
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

		{
			B bi;
			bi.p = bi.p + 1;
			bi.p = bi.p - 1;
		}

		arr.extractRange(10, 20);
		if (!arr.eraseRangeAndShrink(0, 7))
			throw std::bad_alloc();

		str.extractRange(10, 20);
		if (!str.eraseRangeAndShrink(0, 7))
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

	peff::DynArray<B> arr(&peff::g_stdAlloc);
	for (int i = 0; i < 32; i++) {
		int tmp = i + 1048576;
		if (!arr.pushFront(B()))
			throw std::bad_alloc();
	}

	size_t szBuffer = peff::BufferAlloc::calcAllocSize(sizeof(peff::Map<size_t, size_t>::NodeType), alignof(std::max_align_t)) *
					  32,
		   alignment = alignof(std::max_align_t);

	char *b = (char *)peff::g_stdAlloc.alloc(szBuffer, alignment);
	{
		peff::BufferAlloc ba(b, szBuffer);
		peff::Map<size_t, size_t> m(&ba);

		for (size_t i = 0; i < 32; ++i) {
			if (!m.insert(+i, +i))
				std::terminate();
		}

		for (size_t i = 0; i < 32; i += 2) {
			m.remove(i);
		}

		for (auto [k, v] : m) {
			if (k != v)
				std::terminate();
		}
	}
	peff::g_stdAlloc.release(b, szBuffer, alignment);

	{
		char buffer[1024];
		peff::BufferAlloc bufferAlloc(buffer, sizeof(buffer));
		peff::UpstreamedBufferAlloc alloc(&bufferAlloc, &peff::g_stdAlloc);

		for (size_t i = 8; i < 1024; ++i) {
			void *const p1 = alloc.alloc(i, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p1);
			void *const p2 = alloc.alloc(i * 2, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p2);
			memset(p1, 0, i);
			memset(p2, 0, i * 2);
			alloc.release(p1, i, sizeof(std::max_align_t));
			alloc.release(p2, i * 2, sizeof(std::max_align_t));
			void *const p3 = alloc.alloc(i * 3, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p3);
			memset(p3, 0, i * 3);
			alloc.release(p3, i * 3, sizeof(std::max_align_t));
		}
	}

	{
		char buffer[1024];
		peff::BufferAlloc bufferAlloc(buffer, sizeof(buffer));
		peff::UpstreamedBufferAlloc alloc(&bufferAlloc, &peff::g_stdAlloc);

		void *oldP1 = nullptr, *oldP2 = nullptr, *oldP3 = nullptr;

		for (size_t i = 8; i < 1024; ++i) {
			void *const p1 = oldP1 ? alloc.realloc(oldP1, i - 1, sizeof(std::max_align_t), i, sizeof(std::max_align_t)) : alloc.alloc(i, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p1);
			void *const p2 = oldP2 ? alloc.realloc(oldP2, (i - 1) * 2, sizeof(std::max_align_t), i * 2, sizeof(std::max_align_t)) : alloc.alloc(i * 2, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p2);
			memset(p1, 0, i);
			memset(p2, 0, i * 2);
			oldP1 = p1;
			oldP2 = p2;
			void *const p3 = oldP3 ? alloc.realloc(oldP3, (i - 1) * 3, sizeof(std::max_align_t), i * 3, sizeof(std::max_align_t)) : alloc.alloc(i * 3, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p3);
			memset(p3, 0, i * 3);
			oldP3 = p3;
		}

		alloc.release(oldP1, 1023, sizeof(std::max_align_t));
		alloc.release(oldP2, 1023 * 2, sizeof(std::max_align_t));
		alloc.release(oldP3, 1023 * 3, sizeof(std::max_align_t));
	}

	{
		peff::RadixTree<uint32_t, int> test(peff::getDefaultAlloc());

		for (uint32_t i = 0; i < 100; ++i) {
			if (!test.insert(i, i))
				std::terminate();
		}

		for (auto i = test.beginReversed(); i != test.endReversed(); ++i) {
			printf("Value: %d\n", *i);
		}
	}

	{
		peff::BinaryHeapArray<uint32_t> test(peff::getDefaultAlloc());

		for (uint32_t i = 0; i < 10; ++i) {
			if (!test.insert(+i))
				std::terminate();

			puts("------------------");
			for (auto j = test.begin(); j != test.end(); ++j) {
				printf("Value: %u\n", *j);
			}
		}

		for (uint32_t i = 10; i; --i) {
			if (!test.popBackAndShrink())
				std::terminate();

			puts("------------------");
			for (auto j = test.begin(); j != test.end(); ++j) {
				printf("Value: %u\n", *j);
			}
		}
	}

	bool endian = peff::getByteOrder();

	if (endian)
		puts("Big endian");
	else
		puts("Little endian");

	return 0;
}
