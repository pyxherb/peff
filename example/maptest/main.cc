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

	virtual void test_a() {
		puts("test_a");
	}
};

struct Test2 : public peff::SharedFromThis<Test2> {
	std::atomic_size_t ref = 0;

	virtual void on_ref_zero() noexcept = 0;

	size_t inc_ref(size_t global_ref_count) {
		return ++ref;
	}

	size_t dec_ref(size_t global_ref_count) {
		if (!--ref) {
			on_ref_zero();
			return 0;
		}
		return ref;
	}

	uint8_t test2[1024];

	virtual void test_b() {
		puts("test_b");
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
	virtual void on_ref_zero() noexcept {
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

	peff::String s(&peff::g_std_allocator);
	s.resize(3);
	memcpy(s.data(), "123", 3);
	assert(((std::string_view)s) == "123");

	peff::DynArray<SomethingUncopyable> a(&peff::g_std_allocator);

	peff::SharedPtr<RcObj> outer_shared_ptr;

	{
		peff::FallibleHashSet<int, FallibleComparator<int>, FallibleHasher<int>> test(&peff::g_std_allocator);

		for (int i = 0; i < 1024; ++i) {
			if (test.insert(+i)) {
				if (test.size() != 1) {
					std::terminate();
				}
			}
		}
	}
	{
		peff::SharedPtr<RcObj> shared_ptr = peff::make_shared<RcObj>(peff::default_allocator(), "SharedPtr");
		peff::SharedPtr<Test2> test2_ptr = shared_ptr.cast_to<Test2>();

		outer_shared_ptr = shared_ptr;

		auto test2_ptr_locked = peff::WeakPtr<Test2>(test2_ptr).lock();
		auto test2_ptr_from_this = test2_ptr->shared_from_this();

		peff::Set<int> map(&peff::g_std_allocator);
		peff::RcObjectPtr<RcObj> strong_ref;
		strong_ref = new RcObj("StrongRef");

		test = strong_ref;

		strong_ref->test_a();
		strong_ref->test_b();

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
	outer_shared_ptr.reset();
	{
		peff::HashSet<int> map(&peff::g_std_allocator);

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
	}
	{
		peff::Map<int, peff::String> map(&peff::g_std_allocator);

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;

			peff::String s(&peff::g_std_allocator);
			{
				std::string std_string = std::to_string(j);
				if (!s.resize(std_string.size()))
					throw std::bad_alloc();
				memcpy(s.data(), std_string.data(), std_string.size());
			}

			std::string_view sv = (std::string_view)s;
			std::cout << "Dumping string: " << sv << std::endl;

			printf("Inserting: %d\n", j);
			if (!map.insert(std::move(j), std::move(s)))
				throw std::bad_alloc();
		}
	}

	{
		peff::BitArray bit_arr(&peff::g_std_allocator);

		bit_arr.resize_uninit(64);

		bit_arr.fill_set(0, 64);
		bit_arr.fill_clear(0, 48);

		for (size_t i = 0; i < bit_arr.bit_size(); ++i) {
			printf("%s", bit_arr.get_bit(i) ? "1" : "0");
		}

		puts("");
	}

	{
		peff::DynArray<int> arr(&peff::g_std_allocator);
		peff::String str(&peff::g_std_allocator);

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

		arr.extract_range(10, 20);
		if (!arr.erase_range_and_shrink(0, 7))
			throw std::bad_alloc();

		str.extract_range(10, 20);
		if (!str.erase_range_and_shrink(0, 7))
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

	peff::DynArray<B> arr(&peff::g_std_allocator);
	for (int i = 0; i < 32; i++) {
		int tmp = i + 1048576;
		if (!arr.push_front(B()))
			throw std::bad_alloc();
	}

	size_t buffer_size = peff::BufferAlloc::calc_alloc_size(sizeof(peff::Map<size_t, size_t>::NodeType), alignof(std::max_align_t)) *
					  32,
		   alignment = alignof(std::max_align_t);

	char *b = (char *)peff::g_std_allocator.alloc(buffer_size, alignment);
	{
		peff::BufferAlloc ba(b, buffer_size);
		peff::Map<size_t, size_t> m(&ba);

		for (size_t i = 0; i < 32; ++i) {
			if (!m.insert(+i, +i))
				std::terminate();
		}

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
	peff::g_std_allocator.release(b, buffer_size, alignment);

	{
		char buffer[1024];
		peff::BufferAlloc buffer_alloc(buffer, sizeof(buffer));
		peff::UpstreamedBufferAlloc alloc(&buffer_alloc, &peff::g_std_allocator);

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
		peff::BufferAlloc buffer_alloc(buffer, sizeof(buffer));
		peff::UpstreamedBufferAlloc alloc(&buffer_alloc, &peff::g_std_allocator);

		void *old_p1 = nullptr, *old_p2 = nullptr, *old_p3 = nullptr;

		for (size_t i = 8; i < 1024; ++i) {
			void *const p1 = old_p1 ? alloc.realloc(old_p1, i - 1, sizeof(std::max_align_t), i, sizeof(std::max_align_t)) : alloc.alloc(i, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p1);
			void *const p2 = old_p2 ? alloc.realloc(old_p2, (i - 1) * 2, sizeof(std::max_align_t), i * 2, sizeof(std::max_align_t)) : alloc.alloc(i * 2, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p2);
			memset(p1, 0, i);
			memset(p2, 0, i * 2);
			old_p1 = p1;
			old_p2 = p2;
			void *const p3 = old_p3 ? alloc.realloc(old_p3, (i - 1) * 3, sizeof(std::max_align_t), i * 3, sizeof(std::max_align_t)) : alloc.alloc(i * 3, sizeof(std::max_align_t));
			printf("Allocated: %p\n", p3);
			memset(p3, 0, i * 3);
			old_p3 = p3;
		}

		alloc.release(old_p1, 1023, sizeof(std::max_align_t));
		alloc.release(old_p2, 1023 * 2, sizeof(std::max_align_t));
		alloc.release(old_p3, 1023 * 3, sizeof(std::max_align_t));
	}

	{
		peff::RadixTree<uint32_t, int> test(peff::default_allocator());

		for (uint32_t i = 0; i < 100; ++i) {
			if (!test.insert(i, i))
				std::terminate();
		}

		for (auto i = test.begin_reversed(); i != test.end_reversed(); ++i) {
			printf("Value: %d\n", *i);
		}
	}

	{
		peff::BinaryHeapArray<uint32_t> test(peff::default_allocator());

		for (uint32_t i = 0; i < 10; ++i) {
			if (!test.insert(+i))
				std::terminate();

			puts("------------------");
			for (auto j = test.begin(); j != test.end(); ++j) {
				printf("Value: %u\n", *j);
			}
		}

		for (uint32_t i = 10; i; --i) {
			if (!test.pop_back_and_shrink())
				std::terminate();

			puts("------------------");
			for (auto j = test.begin(); j != test.end(); ++j) {
				printf("Value: %u\n", *j);
			}
		}
	}

	bool endian = peff::get_byte_order();

	if (endian)
		puts("Big endian");
	else
		puts("Little endian");

	peff::HashMap<int, peff::String> hm(&peff::g_std_allocator);

	for (int i = 0; i < 16; i++) {
		int j = i & 1 ? i : 32 - i;
		printf("Inserting: %d\n", j);
		peff::String s(&peff::g_std_allocator);

		if(!s.build(std::to_string(j)))
			throw std::bad_alloc();
		if (!hm.insert(+j, std::move(s)))
			throw std::bad_alloc();
	}


	for (int i = 0; i < 16; i++) {
		int j = i & 1 ? i : 32 - i;
		printf("Removing: %d\n", j);

		hm.remove(j);

		auto k = hm.begin();
		while (k != hm.end()) {
			printf("%d: %s\n", k.key(), k.value().data());
			++k;
		}

		// map.dump(std::cout);
	}

	return 0;
}
