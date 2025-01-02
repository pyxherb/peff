#include <cstdio>
#include <peff/containers/set.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <peff/containers/hashset.h>
#include <peff/containers/hashmap.h>

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	peff::StdAlloc myStdAlloc;
	{
		peff::Set<int> map;

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
		peff::Set<int> map2(&myStdAlloc);
		if (!peff::copyAssign(map2, map))
			throw std::bad_alloc();

		auto k = map2.begin();
		while (k != map2.end()) {
			printf("%d\n", *(k++));
		}
	}
	{
		peff::HashSet<int> map;

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
		peff::HashMap<int, int> map;

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			int k = i;
			printf("Inserting: %d\n", j);
			if (!map.insert(std::move(j), std::move(k)))
				throw std::bad_alloc();
		}

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Removing: %d\n", j);

			map.remove(j);

			auto k = map.begin();
			while (k != map.end()) {
				printf("%d=%d\n", k.key(), k.value());
				++k;
			}

			// map.dump(std::cout);
		}
	}

	{
		peff::DynArray<int> arr;
		peff::String str;

		for (int i = 0; i < 32; i++) {
			arr.insert(0, i);
			str.insert(0, 'a' + i);

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
		arr.eraseRange(0, 7);

		str.extractRange(10, 20);
		str.eraseRange(0, 7);

		for (size_t i = 0; i < arr.size(); ++i) {
			printf("%d ", arr.at(i));
		}

		puts("");

		for (size_t i = 0; i < str.size(); ++i) {
			printf("%c ", str.at(i));
		}

		puts("");
	}

	return 0;
}
