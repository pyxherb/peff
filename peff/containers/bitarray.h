#ifndef _PEFF_CONTAINERS_BITARRAY_H_
#define _PEFF_CONTAINERS_BITARRAY_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

namespace peff {
	class BitArray {
	private:
		size_t _nBits = 0;
		uint8_t *_buffer = nullptr;
		Alloc *_allocator;

	public:
		PEFF_FORCEINLINE BitArray(Alloc *allocator = getDefaultAlloc()) : _allocator(allocator) {
		}
		PEFF_FORCEINLINE ~BitArray() {
			_allocator->release(_buffer, size(), 1);
		}

		PEFF_FORCEINLINE size_t size() {
			return (_nBits + 7) >> 3;
		}

		PEFF_FORCEINLINE size_t bitSize() {
			return _nBits;
		}

		PEFF_FORCEINLINE bool resize(size_t nBits) {
			if (nBits) {
				const size_t nBytes = (nBits + 7) >> 3;

				uint8_t *newBuffer = (uint8_t *)_allocator->alloc(nBytes, 1);
				if (!newBuffer)
					return false;

				if (_buffer) {
					memcpy((void *)newBuffer, (void *)_buffer, nBytes);
					newBuffer[nBytes - 1] &= ~(0xff >> (nBits & 7));
				}
				_allocator->release(_buffer, size(), 1);

				_buffer = newBuffer;
			} else {
				_allocator->release(_buffer, size(), 1);
			}
			_nBits = nBits;

			return true;
		}

		PEFF_FORCEINLINE uint8_t *data() {
			return _buffer;
		}

		PEFF_FORCEINLINE void setBit(size_t bitIndex) {
			assert(bitIndex < _nBits);

			_buffer[(bitIndex >> 3)] |= (1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE void clearBit(size_t bitIndex) {
			assert(bitIndex < _nBits);

			_buffer[(bitIndex >> 3)] &= ~(1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE bool getBit(size_t bitIndex) {
			return (_buffer[(bitIndex >> 3)] >> (bitIndex & 7)) & 1;
		}

		PEFF_FORCEINLINE void fillSet(size_t bitIndex, size_t nBits) {
			assert(bitIndex < _nBits);
			assert(bitIndex + nBits <= _nBits);

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
			assert(bitIndex < _nBits);
			assert(bitIndex + nBits < _nBits);

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

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator;
		}
	};
}

#endif
