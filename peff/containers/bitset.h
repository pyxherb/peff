#ifndef _PEFF_CONTAINERS_BITSET_H_
#define _PEFF_CONTAINERS_BITSET_H_

#include "basedefs.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

namespace peff {
	template <size_t N>
	class BitSet {
	public:
		constexpr static size_t BIT_SIZE = N;
		constexpr static size_t BYTE_SIZE = (N + (8 - 1)) / 8;

	private:
		uint8_t _buffer[BYTE_SIZE] = { 0 };

		PEFF_FORCEINLINE void _setBit(size_t bitIndex) {
			_buffer[(bitIndex >> 3)] |= (1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE void _clearBit(size_t bitIndex) {
			_buffer[(bitIndex >> 3)] &= ~(1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE void _setByte(size_t bitIndex, uint8_t b, uint8_t nBits) {
			size_t index = (bitIndex >> 3), inBitIndex = (bitIndex & 7);

			if ((bitIndex + (nBits - 1)) >> 3 == index) {
				// Does not across the byte boundary and inside a single byte.
				// Such as 000|111|00
				uint8_t mask = ~(((1 << nBits) - 1) << inBitIndex);
				_buffer[index] &= mask;
				_buffer[index] |= b << inBitIndex;
			} else {
				// Acrosses the byte boundary.
				// Such as 000|10010 110|00000
				uint8_t mask = 0xff >> (8 - inBitIndex);
				_buffer[index] &= mask;
				_buffer[index] |= b << inBitIndex;

				uint8_t bitsRemaining = nBits - (8 - inBitIndex);
				mask = 0xff << bitsRemaining;
				_buffer[index + 1] &= mask;
				_buffer[index + 1] |= b >> (8 - inBitIndex);
			}

			// Used for verifying if the set byte works correctly
			uint8_t test = _getByte(bitIndex, nBits);
			if (nBits < 8) {
				test &= (0xff >> (8 - nBits));
				b &= (0xff >> (8 - nBits));
			}
			assert(test == b);
		}

		PEFF_FORCEINLINE uint8_t _getByte(size_t bitIndex, uint8_t nBits) const {
			if (bitIndex & 7) {
				size_t byteIndex = bitIndex >> 3, bitOffset = bitIndex & 7;
				if (((bitIndex + (nBits - 1)) >> 3) > byteIndex) {
					uint8_t l = _buffer[byteIndex] >> (bitOffset), h = _buffer[byteIndex + 1] << (8 - bitOffset);
					return (l | h) & (0xff >> (8 - nBits));
				} else {
					return (_buffer[byteIndex] >> (bitOffset)) & (0xff >> (8 - nBits));
				}
			}
			return _buffer[bitIndex >> 3] & (0xff >> (8 - nBits));
		}

	public:
		PEFF_FORCEINLINE BitSet() {
		}
		PEFF_FORCEINLINE ~BitSet() = default;

		PEFF_FORCEINLINE size_t size() const {
			return BYTE_SIZE;
		}

		PEFF_FORCEINLINE size_t bitSize() const {
			return N;
		}

		PEFF_FORCEINLINE uint8_t *data() const {
			return _buffer;
		}

		PEFF_FORCEINLINE void setBit(size_t bitIndex) {
			assert(bitIndex < N);
			_setBit(bitIndex);
		}

		PEFF_FORCEINLINE void clearBit(size_t bitIndex) {
			assert(bitIndex < N);
			_clearBit(bitIndex);
		}

		PEFF_FORCEINLINE bool getBit(size_t bitIndex) const {
			assert(bitIndex < N);
			return (_buffer[(bitIndex >> 3)] >> (bitIndex & 7)) & 1;
		}

		PEFF_FORCEINLINE void fillSet(size_t bitIndex, size_t nBits) {
			assert(bitIndex < N);
			assert(bitIndex + nBits <= N);

			size_t idxEnd = bitIndex + nBits;

			size_t idxStartFillByte = (bitIndex + 7) >> 3;
			size_t idxEndFillByte = idxEnd >> 3;
			size_t szFillByte = idxEndFillByte - idxStartFillByte;

			if (bitIndex & 7) {
				uint8_t startBits = 0xff << (bitIndex & 7);
				_buffer[bitIndex >> 3] |= startBits;
			}

			if (szFillByte)
				memset(_buffer + idxStartFillByte, 0xff, szFillByte);

			if (idxEnd & 7) {
				uint8_t endBits = 0xff >> (7 - (bitIndex & 7));
				_buffer[idxEnd >> 3] |= endBits;
			}
		}

		PEFF_FORCEINLINE void fillClear(size_t bitIndex, size_t nBits) {
			assert(bitIndex < N);
			assert(bitIndex + nBits < N);

			size_t idxEnd = bitIndex + nBits;

			size_t idxStartFillByte = (bitIndex + 7) >> 3;
			size_t idxEndFillByte = idxEnd >> 3;
			size_t szFillByte = idxEndFillByte - idxStartFillByte;

			if (bitIndex & 7) {
				uint8_t startBits = 0xff >> (7 - (bitIndex & 7));
				_buffer[bitIndex >> 3] &= startBits;
			}

			if (szFillByte)
				memset(_buffer + idxStartFillByte, 0x00, szFillByte);

			if (idxEnd & 7) {
				uint8_t endBits = 0xff << (bitIndex & 7);
				_buffer[idxEnd >> 3] &= endBits;
			}
		}

		PEFF_FORCEINLINE void setByte(size_t bitIndex, uint8_t b) {
			assert(bitIndex + 8 <= N);

			_setByte(bitIndex, b, 8);
		}

		PEFF_FORCEINLINE void setByte(size_t bitIndex, uint8_t b, uint8_t nBits) {
			assert(bitIndex + nBits <= N);

			_setByte(bitIndex, b, nBits);
		}

		PEFF_FORCEINLINE uint8_t getByte(size_t bitIndex) const {
			assert(bitIndex + 8 <= N);

			return _getByte(bitIndex, 8);
		}

		PEFF_FORCEINLINE uint8_t getByte(size_t bitIndex, uint8_t nBits) const {
			assert(nBits);
			assert(nBits <= 8);
			assert(bitIndex + nBits <= N);

			return _getByte(bitIndex, nBits);
		}

		PEFF_FORCEINLINE void getBytes(char *buf, size_t len, size_t bitIndex) const {
			assert(bitIndex + 8 * len <= N);

			if (!((bitIndex) & 7)) {
				memcpy(buf, _buffer + bitIndex / 8, len);
			} else {
				size_t curIndex = bitIndex;
				for (size_t i = 0; i < len; ++i) {
					buf[i] = _getByte(curIndex, 8);
					curIndex += 8;
				}
			}
		}
	};
}

#endif
