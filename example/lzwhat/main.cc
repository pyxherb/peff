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

bool lzwhatEncodeVarInt64(peff::BitArray &bitArray, uint64_t data) {
	size_t beginning = bitArray.bitSize();
	if (data < (1llu << 7)) {
		uint8_t b = 0b1;

		if (!bitArray.resizeUninitialized(beginning + 1 + 7)) {
			return false;
		}
		bitArray.setByte(beginning, b, 1);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 1, a, 7);
	} else if (data < (1llu << 14)) {
		uint8_t b = 0b01;

		if (!bitArray.resizeUninitialized(beginning + 2 + 14)) {
			return false;
		}
		bitArray.setByte(beginning, b, 2);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 2, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 2 + 7, a, 7);
	} else if (data < (1llu << 21)) {
		uint8_t b = 0b001;

		if (!bitArray.resizeUninitialized(beginning + 3 + 21)) {
			return false;
		}
		bitArray.setByte(beginning, b, 3);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 3, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 3 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 3 + 14, a, 7);
	} else if (data < (1llu << 28)) {
		uint8_t b = 0b0001;

		if (!bitArray.resizeUninitialized(beginning + 4 + 28)) {
			return false;
		}
		bitArray.setByte(beginning, b, 4);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 4, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 4 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 4 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 4 + 21, a, 7);
	} else if (data < (1llu << 35)) {
		uint8_t b = 0b00001;

		if (!bitArray.resizeUninitialized(beginning + 5 + 35)) {
			return false;
		}
		bitArray.setByte(beginning, b, 5);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 5, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 5 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 5 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 5 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bitArray.setByte(beginning + 5 + 28, a, 7);
	} else if (data < (1llu << 42)) {
		uint8_t b = 0b000001;

		if (!bitArray.resizeUninitialized(beginning + 6 + 42)) {
			return false;
		}
		bitArray.setByte(beginning, b, 6);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 6, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 6 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 6 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 6 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bitArray.setByte(beginning + 6 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bitArray.setByte(beginning + 6 + 35, a, 7);
	} else if (data < (1llu << 49)) {
		uint8_t b = 0b0000001;

		if (!bitArray.resizeUninitialized(beginning + 7 + 49)) {
			return false;
		}
		bitArray.setByte(beginning, b, 7);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 7, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 7 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 7 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 7 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bitArray.setByte(beginning + 7 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bitArray.setByte(beginning + 7 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bitArray.setByte(beginning, a, 7);
	} else if (data < (1llu << 56)) {
		uint8_t b = 0b00000001;

		if (!bitArray.resizeUninitialized(beginning + 8 + 56)) {
			return false;
		}
		bitArray.setByte(beginning, b, 8);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 8, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 8 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 8 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 8 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bitArray.setByte(beginning + 8 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bitArray.setByte(beginning + 8 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bitArray.setByte(beginning + 8 + 42, a, 7);
		a = (data >> 49) & 0b1111111;
		bitArray.setByte(beginning + 8 + 49, a, 7);
	} else {
		uint8_t b = 0b00000000;

		if (!bitArray.resizeUninitialized(beginning + 9 + 64)) {
			return false;
		}
		bitArray.setByte(beginning, b, 8);

		uint8_t a;
		a = data & 0b1111111;
		bitArray.setByte(beginning + 8, a, 7);
		a = (data >> 7) & 0b1111111;
		bitArray.setByte(beginning + 8 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bitArray.setByte(beginning + 8 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bitArray.setByte(beginning + 8 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bitArray.setByte(beginning + 8 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bitArray.setByte(beginning + 8 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bitArray.setByte(beginning + 8 + 42, a, 7);
		a = (data >> 49) & 0b1111111;
		bitArray.setByte(beginning + 8 + 49, a, 7);
		a = (data >> 56) & 0b1111111;
		bitArray.setByte(beginning + 8 + 56, a, 8);
	}

	return true;
}

bool lzwhatDecodeVarInt64(const peff::BitArray &bitArray, size_t &curIndex, uint64_t &dataOut) {
	dataOut = 0;

	auto readFirstBits = [](const peff::BitArray &bitArray, size_t &curIndex, size_t &dataOut) -> bool {
		if (curIndex + 7 > bitArray.bitSize()) {
			return false;
		}
		uint8_t a;
		a = bitArray.getByte(curIndex, 7);
		dataOut = a;
		curIndex += 7;
		return true;
	};
	auto shiftAndRead = [](const peff::BitArray &bitArray, size_t &curIndex, size_t &dataOut) -> bool {
		if (curIndex + 7 > bitArray.bitSize()) {
			return false;
		}
		uint8_t a;
		a = bitArray.getByte(curIndex, 7);
		dataOut <<= 7;
		dataOut |= a;
		curIndex += 7;
		return true;
	};
	auto readLastBits = [](const peff::BitArray &bitArray, size_t &curIndex, size_t &dataOut) -> bool {
		if (curIndex + 8 > bitArray.bitSize()) {
			return false;
		}
		uint8_t a;
		a = bitArray.getByte(curIndex, 8);
		dataOut <<= 7;
		dataOut = a;
		curIndex += 8;
		return true;
	};

	if (curIndex + 1 > bitArray.bitSize()) {
		return false;
	}
	if (!bitArray.getBit(curIndex++)) {
		if (curIndex + 1 > bitArray.bitSize()) {
			return false;
		}
		if (!bitArray.getBit(curIndex++)) {
			if (curIndex + 1 > bitArray.bitSize()) {
				return false;
			}
			if (!bitArray.getBit(curIndex++)) {
				if (curIndex + 1 > bitArray.bitSize()) {
					return false;
				}
				if (!bitArray.getBit(curIndex++)) {
					if (curIndex + 1 > bitArray.bitSize()) {
						return false;
					}
					if (!bitArray.getBit(curIndex++)) {
						if (curIndex + 1 > bitArray.bitSize()) {
							return false;
						}
						if (!bitArray.getBit(curIndex++)) {
							if (curIndex + 1 > bitArray.bitSize()) {
								return false;
							}
							if (!bitArray.getBit(curIndex++)) {
								if (curIndex + 1 > bitArray.bitSize()) {
									return false;
								}
								if (!bitArray.getBit(curIndex++)) {
									// 64-bit
									if (!readFirstBits(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!readLastBits(bitArray, curIndex, dataOut))
										return false;
								} else {
									// 56-bit
									if (!readFirstBits(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
									if (!shiftAndRead(bitArray, curIndex, dataOut))
										return false;
								}
							} else {
								// 49-bit
								if (!readFirstBits(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
								if (!shiftAndRead(bitArray, curIndex, dataOut))
									return false;
							}
						} else {
							// 42-bit
							if (!readFirstBits(bitArray, curIndex, dataOut))
								return false;
							if (!shiftAndRead(bitArray, curIndex, dataOut))
								return false;
							if (!shiftAndRead(bitArray, curIndex, dataOut))
								return false;
							if (!shiftAndRead(bitArray, curIndex, dataOut))
								return false;
							if (!shiftAndRead(bitArray, curIndex, dataOut))
								return false;
							if (!shiftAndRead(bitArray, curIndex, dataOut))
								return false;
						}
					} else {
						// 35-bit
						if (!readFirstBits(bitArray, curIndex, dataOut))
							return false;
						if (!shiftAndRead(bitArray, curIndex, dataOut))
							return false;
						if (!shiftAndRead(bitArray, curIndex, dataOut))
							return false;
						if (!shiftAndRead(bitArray, curIndex, dataOut))
							return false;
						if (!shiftAndRead(bitArray, curIndex, dataOut))
							return false;
					}
				} else {
					// 28-bit
					if (!readFirstBits(bitArray, curIndex, dataOut))
						return false;
					if (!shiftAndRead(bitArray, curIndex, dataOut))
						return false;
					if (!shiftAndRead(bitArray, curIndex, dataOut))
						return false;
					if (!shiftAndRead(bitArray, curIndex, dataOut))
						return false;
				}
			} else {
				// 21-bit
				if (!readFirstBits(bitArray, curIndex, dataOut))
					return false;
				if (!shiftAndRead(bitArray, curIndex, dataOut))
					return false;
				if (!shiftAndRead(bitArray, curIndex, dataOut))
					return false;
			}
		} else {
			// 14-bit
			if (!readFirstBits(bitArray, curIndex, dataOut))
				return false;
			if (!shiftAndRead(bitArray, curIndex, dataOut))
				return false;
		}
	} else {
		// 7-bit
		if (!readFirstBits(bitArray, curIndex, dataOut))
			return false;
	}

	return true;
}

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
					if (!lzwhatEncodeVarInt64(bitArray, disp)) {
						return false;
					}
					if (!lzwhatEncodeVarInt64(bitArray, len)) {
						return false;
					}
					/* if (!bitArray.pushBackBytes((char *)&disp, sizeof(disp)))
						return false;
					if (!bitArray.pushBackBytes((char *)&len, sizeof(len)))
						return false;*/
					i += len - 1;
					goto next;
				}
			}
		}

		if (!bitArray.pushBack(true))
			return false;
		if (!bitArray.pushBackByte(*(uint8_t *)matchBufPtr))
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

#define INC_CUR_PTR(i, size, bitArray) \
	if (((i) += (size)) > (bitArray).bitSize()) return false;

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
			if (!lzwhatDecodeVarInt64(bitArray, i, disp)) {
				return false;
			}
			/* bitArray.getBytes((char *)&disp, sizeof(disp), i);*/
			assert(disp <= dictSize);
			/* bitArray.getBytes((char *)&len, sizeof(len), i);*/
			if (!lzwhatDecodeVarInt64(bitArray, i, len)) {
				return false;
			}
			assert(len <= dictSize);
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
