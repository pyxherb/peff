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

enum class LZWhatDataType : uint8_t {
	RawByte,
	Replication
};

bool lzwhatCompress(peff::BitArray &bitArray, const char *buf, size_t size, size_t dictSize, size_t encodeBufferSize) {
	const char *const bufLimit = buf + size;

	for (size_t i = 0; i < size; ++i) {
		const char *searchBufPtr,
			*searchEndPtr;

		if (i < dictSize) {
			searchBufPtr = buf;
		} else {
			searchBufPtr = buf + i - dictSize;
		}
		searchEndPtr = buf + i;

		const char *matchBufPtr = searchEndPtr,
				   *matchEndPtr = matchBufPtr + encodeBufferSize;
		if (matchEndPtr > bufLimit) {
			matchEndPtr = bufLimit;
		}
		size_t searchBufSize = searchEndPtr - searchBufPtr,
			   matchBufSize = matchEndPtr - matchBufPtr;

		for (size_t matchLen = searchBufSize < matchBufSize ? searchBufSize : matchBufSize; matchLen > 0; --matchLen) {
			for (size_t offStart = 0; offStart < searchBufSize - matchLen; ++offStart) {
				if (!memcmp(searchBufPtr + offStart, matchBufPtr, matchLen)) {
					size_t disp = (searchEndPtr - (searchBufPtr + offStart));
					size_t len = matchLen;
					printf("(%zu, %zu)\n", disp, len);
					if (!bitArray.pushBack(false))
						return false;
					if (!bitArray.pushBackBytes((char *)&disp, sizeof(disp)))
						return false;
					if (!bitArray.pushBackBytes((char *)&len, sizeof(len)))
						return false;
					i += len - 1;
					goto next;
				}
			}
		}

		if (!bitArray.pushBack(true))
			return false;
		if (!bitArray.pushBackByte(*(uint8_t*)matchBufPtr))
			return false;
		printf("'%c'\n", *(uint8_t *)matchBufPtr);
	next:;
	}

	return true;
}

bool lzwhatDecompress(const peff::BitArray &bitArray, peff::Alloc *allocator, size_t dictSize, peff::DynArray<char> &dataOut) {
	size_t i = 0;

	peff::DynArray<char> dict(allocator);

	if (!dict.resize(dictSize)) {
		return false;
	}

	size_t curDictSize = 0, curDictIndex = 0;

	auto calcDictIndex = [](size_t &curDictIndex, size_t delta, size_t dictSize) {
		assert(delta < dictSize);
		if (curDictIndex + delta >= dictSize) {
			curDictIndex = (curDictIndex + delta) - dictSize;
		} else {
			curDictIndex += delta;
		}
		assert(curDictIndex < dictSize);
	};
	auto copyToDict = [](char *dict, size_t dictSize, size_t curDictIndex, const char *in, size_t len) {
		if (curDictIndex + len > dictSize) {
			memcpy(dict + curDictIndex, in, dictSize - curDictIndex);
			memcpy(dict, in + (dictSize - curDictIndex), curDictIndex + len - dictSize);
		} else {
			memcpy(dict + curDictIndex, in, len);
		}
	};
	auto copyFromDict = [](char *dict, size_t dictSize, size_t curDictIndex, char *out, size_t len) {
		if (curDictIndex + len > dictSize) {
			memcpy(out, dict + curDictIndex, dictSize - curDictIndex);
			memcpy(out + (dictSize - curDictIndex), dict, curDictIndex + len - dictSize);
		} else {
			memcpy(out, dict + curDictIndex, len);
		}
	};

	#define INC_CUR_PTR(i, size, bitArray) if (((i) += (size)) > (bitArray).bitSize()) return false;

	while (i < bitArray.bitSize()) {
		bool b = bitArray.getBit(i);
		INC_CUR_PTR(i, 1, bitArray);

		if (b) {
			uint8_t c = bitArray.getByte(i);
			if (!dataOut.pushBack(+*(char *)&c)) {
				return false;
			}
			INC_CUR_PTR(i, 8, bitArray);
			copyToDict(dict.data(), dictSize, curDictIndex, (char *)&c, 1);
			calcDictIndex(curDictIndex, 1, dictSize);
		} else {
			size_t start = dataOut.size();
			size_t disp;
			size_t len;
			bitArray.getBytes((char *)&disp, sizeof(disp), i);
			assert(disp <= dictSize);
			INC_CUR_PTR(i, sizeof(size_t) * 8, bitArray);
			bitArray.getBytes((char *)&len, sizeof(len), i);
			assert(len <= dictSize);
			INC_CUR_PTR(i, sizeof(size_t) * 8, bitArray);
			if (!dataOut.resizeUninitialized(dataOut.size() + len)) {
				return false;
			}
			size_t leftLimit = curDictIndex;
			calcDictIndex(leftLimit, dictSize - disp, dictSize);
			copyFromDict(dict.data(), dictSize, leftLimit, dataOut.data() + start, len);
			copyToDict(dict.data(), dictSize, curDictIndex, dataOut.data() + start, len);
			calcDictIndex(curDictIndex, len, dictSize);
		}
	}

	return true;
}

int main() {
	const char s[] = "ABACBAABCBCCA";

	peff::BitArray bitArray(peff::getDefaultAlloc());

	if (!lzwhatCompress(bitArray, s, sizeof(s) - 1, 7, 4)) {
		std::terminate();
	}

	peff::DynArray<char> data(peff::getDefaultAlloc());

	if (!lzwhatDecompress(bitArray, peff::getDefaultAlloc(), 7, data)) {
		std::terminate();
	}

	for (size_t i = 0; i < data.size(); ++i) {
		putchar(data.at(i));
	}

	return 0;
}
