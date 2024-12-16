#include <cstdio>
#include <peff/containers/set.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/list.h>

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	{
		peff::Set<int> map;

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Inserting: %d\n", j);
			if (!map.insert(j))
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
		peff::Set<int> map2;
		if (!peff::copyAssign(map2, map))
			throw std::bad_alloc();

		auto k = map2.begin();
		while (k != map2.end()) {
			printf("%d\n", *(k++));
		}
	}

	{
		peff::DynArray<int> arr;

		for (int i = 0; i < 32; i++) {
			arr.insert(0, i);

			for (size_t i = 0; i < arr.getSize(); ++i) {
				printf("%d ", arr.at(i));
			}

			puts("");
		}

		arr.extractRange(10, 20);
		//arr.eraseRange(1, 30);

		for (size_t i = 0; i < arr.getSize(); ++i) {
			printf("%d ", arr.at(i));
		}

		puts("");
	}

	return 0;
}
