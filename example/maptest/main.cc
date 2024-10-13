#include <cstdio>
#include <phul/containers/set.h>
#include <phul/containers/dynarray.h>
#include <phul/containers/list.h>

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	{
		phul::Set<int> map;

		for (int i = 0; i < 16; i++) {
			int j = i & 1 ? i : 32 - i;
			printf("Inserting: %d\n", j);
			if (!map.insert(j))
				throw std::bad_alloc();

			map.verify();
		}

		/*
		for (int i = 0; i < 64; i++) {
			int j = i & 1 ? i : 128 - i;
			printf("Removing: %d\n", j);

			map.remove(j);

			auto k = map.begin();
			while (k != map.end()) {
				printf("%d\n", *(k++));
			}

			// map.dump(std::cout);

			map.verify();
		}*/
		/*
		phul::Set<int> map2;
		if (!map.copy(map2))
			throw std::bad_alloc();

		auto k = map2.begin();
		while (k != map2.end()) {
			printf("%d\n", *(k++));
		}*/
	}

	{ /*
		phul::DynArray<int> arr;

		for (int i = 0; i < 64; i++) {
			int j;
			if (i & 1) {
				j = arr.getSize() - i;
			} else {
				j = arr.getSize();
			}
			arr.insertFront(j, i);
		}*/
	}

	return 0;
}
