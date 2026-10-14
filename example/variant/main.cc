#include <cstdio>
#include <peff/utils/byteord.h>
#include <peff/utils/span.h>
#include <peff/containers/set.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <peff/containers/hashset.h>
#include <peff/containers/hashmap.h>
#include <peff/containers/radix_map.h>
#include <peff/containers/map.h>
#include <peff/containers/bitarray.h>
#include <peff/containers/binary_heap.h>
#include <peff/advutils/shared_ptr.h>
#include <peff/advutils/buffer_alloc.h>
#include <peff/containers/btree_map.h>
#include <peff/utils/variant.h>
#include <iostream>
#include <string>
#include <random>

struct SomethingUncopyable {
	peff::String s;
};

int main() {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	peff::Variant<int, char, size_t, float> v = (int)123;

	switch (v.index()) {
		case decltype(v)::CASE_INDEX<int>:
			printf("%d: int\n", v.get<int>());
			break;
		case decltype(v)::CASE_INDEX<char>:
			printf("%c: char\n", v.get<char>());
			break;
		case decltype(v)::CASE_INDEX<size_t>:
			printf("%zu: size_t\n", v.get<size_t>());
			break;
		case decltype(v)::CASE_INDEX<float>:
			printf("%f: float\n", v.get<float>());
			break;
	}

	static_assert(std::is_trivially_destructible_v<peff::Variant<int, char, size_t, float>>);
#if PEFF_CPLUSPLUS >= 202002L
	static_assert(std::is_trivially_copy_constructible_v<peff::Variant<int, char, size_t, float>>);
#endif

	peff::Variant<std::string, peff::String> v2 = (std::string) "test";

	peff::_VariantStorage<true, true, std::string, peff::String>::_TAG_OF<std::string>;
	peff::_VariantStorage<true, true, std::string, peff::String>::_INVALID_TAG_VALUE;

	decltype(v2)::CASE_INDEX<std::string>;

	switch (v.index()) {
		case decltype(v2)::CASE_INDEX<std::string>:
			printf("%s: string\n", v2.get<std::string>().data());
			break;
		case decltype(v2)::CASE_INDEX<peff::String>:
			printf("%s: String\n", v2.get<peff::String>().data());
			break;
	}

	peff::String s(peff::default_allocator());

	if (!s.build("hello"))
		throw std::bad_alloc();
	v2 = std::move(s);

	switch (v2.index()) {
		case decltype(v2)::CASE_INDEX<std::string>:
			printf("%s: string\n", v2.get<std::string>().data());
			break;
		case decltype(v2)::CASE_INDEX<peff::String>:
			printf("%s: String\n", v2.get<peff::String>().data());
			break;
	}

	return 0;
}
