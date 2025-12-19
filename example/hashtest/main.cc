#include <cstdio>
#include <peff/advutils/buffer_alloc.h>
#include <peff/containers/dynarray.h>
#include <peff/utils/hash.h>
#include <iostream>
#include <string>

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	printf("%llu\n", peff::cityHash64("16", sizeof("16") - 1));

	return 0;
}
