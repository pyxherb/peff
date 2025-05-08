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
		size_t _capacity = 0;
		uint8_t *_buffer = nullptr;
		Alloc *_allocator;

		PEFF_FORCEINLINE bool _autoResizeCapacity(size_t nBits) {
			if (nBits < (_capacity >> 1)) {
				if (!reserve(nBits)) {
					return false;
				}
			} else if (nBits > _capacity) {
				if (!reserve(nBits)) {
					return false;
				}
			}
			return true;
		}

		PEFF_FORCEINLINE bool _resizeOrExpandUninitialized(size_t nBits) {
			if (nBits > _capacity) {
				if (!reserve(nBits)) {
					return false;
				}
			}
			return true;
		}

		PEFF_FORCEINLINE bool _resizeOrExpand(size_t nBits, bool initValue) {
			if (nBits > _capacity) {
				if (!reserve(nBits)) {
					return false;
				}
			} else {
				if (nBits > _nBits) {
					if (initValue) {
						fillSet(_nBits, nBits - _nBits);
					} else {
						fillClear(_nBits, nBits - _nBits);
					}
				}
			}
			return true;
		}

		PEFF_FORCEINLINE void _setBit(size_t bitIndex) {
			_buffer[(bitIndex >> 3)] |= (1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE void _clearBit(size_t bitIndex) {
			_buffer[(bitIndex >> 3)] &= ~(1 << (bitIndex & 7));
		}

		PEFF_FORCEINLINE void _setByte(size_t bitIndex, uint8_t b) {
			size_t index = (bitIndex >> 3), inBitIndex = (bitIndex & 7);

			switch (inBitIndex) {
				case 7:
					_buffer[(bitIndex >> 3)] &= (0xff >> 1);
					_buffer[(bitIndex >> 3)] |= (b << 7);
					break;
				case 6:
					_buffer[(bitIndex >> 3)] &= (0xff >> 2);
					_buffer[(bitIndex >> 3)] |= (b << 6);
					break;
				case 5:
					_buffer[(bitIndex >> 3)] &= (0xff >> 3);
					_buffer[(bitIndex >> 3)] |= (b << 5);
					break;
				case 4:
					_buffer[(bitIndex >> 3)] &= (0xff >> 4);
					_buffer[(bitIndex >> 3)] |= (b << 4);
					break;
				case 3:
					_buffer[(bitIndex >> 3)] &= (0xff >> 5);
					_buffer[(bitIndex >> 3)] |= (b << 3);
					break;
				case 2:
					_buffer[(bitIndex >> 3)] &= (0xff >> 6);
					_buffer[(bitIndex >> 3)] |= (b << 2);
					break;
				case 1:
					_buffer[(bitIndex >> 3)] &= (0xff >> 7);
					_buffer[(bitIndex >> 3)] |= (b << 1);
					break;
				case 0:
					_buffer[(bitIndex >> 3)] = b;
					break;
			}

			size_t tailSize = inBitIndex;

			switch (tailSize) {
				case 7:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 7);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 1);
					break;
				case 6:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 6);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 2);
					break;
				case 5:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 5);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 3);
					break;
				case 4:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 4);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 4);
					break;
				case 3:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 3);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 5);
					break;
				case 2:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 2);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 6);
					break;
				case 1:
					_buffer[(bitIndex >> 3) + 1] &= (0xff << 1);
					_buffer[(bitIndex >> 3) + 1] |= (b >> 7);
					break;
				case 0:
					break;
			}

			uint8_t test = _getByte(bitIndex);
			assert(test == b);
		}

		PEFF_FORCEINLINE uint8_t _getByte(size_t bitIndex) const {
			if (bitIndex & 7) {
				size_t byteIndex = bitIndex >> 3, bitOffset = bitIndex & 7;
				return (_buffer[byteIndex] >> (bitOffset)) | ((_buffer[byteIndex + 1] << (8 - bitOffset)));
			}
			return _buffer[bitIndex >> 3];
		}

	public:
		PEFF_FORCEINLINE BitArray(Alloc *allocator) : _allocator(allocator) {
		}
		PEFF_FORCEINLINE ~BitArray() {
			if (_buffer)
				_allocator->release(_buffer, size(), 1);
		}

		PEFF_FORCEINLINE size_t size() const {
			return (_nBits + 7) >> 3;
		}

		PEFF_FORCEINLINE size_t bitSize() const {
			return _nBits;
		}

		PEFF_FORCEINLINE bool reserve(size_t nBits) {
			if (nBits) {
				const size_t nBytes = (nBits + 7) >> 3;

				uint8_t *newBuffer = (uint8_t *)_allocator->alloc(nBytes, 1);
				if (!newBuffer)
					return false;

				if (_buffer) {
					if (nBytes > size()) {
						memcpy((void *)newBuffer, (void *)_buffer, size());
					} else {
						memcpy((void *)newBuffer, (void *)_buffer, nBytes);
					}
				}
				if (_buffer)
					_allocator->release(_buffer, size(), 1);

				_buffer = newBuffer;
			} else {
				if (_buffer)
					_allocator->release(_buffer, size(), 1);
			}
			_capacity = ((nBits + 7) >> 3) << 3;

			return true;
		}

		PEFF_FORCEINLINE bool resize(size_t nBits) {
		}

		PEFF_FORCEINLINE bool pushBack(bool bit) {
			if (!_resizeOrExpandUninitialized(_nBits + 1)) {
				return false;
			}
			if (bit) {
				_setBit(_nBits);
			} else {
				_clearBit(_nBits);
			}
			++_nBits;

			return true;
		}

		PEFF_FORCEINLINE bool pushBackByte(uint8_t b) {
			if (!_resizeOrExpandUninitialized(_nBits + 8)) {
				return false;
			}
			_setByte(_nBits, b);
			_nBits += 8;

			return true;
		}

		PEFF_FORCEINLINE bool pushBackBytes(char* data, size_t len) {
			if (!_resizeOrExpandUninitialized(_nBits + len * 8)) {
				return false;
			}
			for (size_t i = 0; i < len; ++i) {
				_setByte(_nBits, data[i]);
				_nBits += 8;
			}
			return true;
		}

		PEFF_FORCEINLINE void popBack() {
			--_nBits;
		}

		PEFF_FORCEINLINE void popByte() {
			_nBits -= 8;
		}

		PEFF_FORCEINLINE bool popBackAndResizeCapacity() {
			if (!_autoResizeCapacity(_nBits - 1)) {
				return false;
			}
			--_nBits;

			return true;
		}

		PEFF_FORCEINLINE bool popBackByteAndResizeCapacity() {
			if (!_autoResizeCapacity(_nBits - 8)) {
				return false;
			}
			_nBits -= 8;

			return true;
		}

		PEFF_FORCEINLINE uint8_t *data() const {
			return _buffer;
		}

		PEFF_FORCEINLINE void setBit(size_t bitIndex) {
			assert(bitIndex < _nBits);
			_setBit(bitIndex);
		}

		PEFF_FORCEINLINE void clearBit(size_t bitIndex) {
			assert(bitIndex < _nBits);
			_clearBit(bitIndex);
		}

		PEFF_FORCEINLINE bool getBit(size_t bitIndex) const {
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

		PEFF_FORCEINLINE void setByte(size_t bitIndex, uint8_t b) {
			assert(bitIndex + 8 <= _nBits);

			_setByte(bitIndex, b);
		}

		PEFF_FORCEINLINE uint8_t getByte(size_t bitIndex) const {
			assert(bitIndex + 8 <= _nBits);

			return _getByte(bitIndex);
		}

		PEFF_FORCEINLINE void getBytes(char *buf, size_t len, size_t bitIndex) const {
			assert(bitIndex + 8 * len <= _nBits);

			if (!((bitIndex) & 7)) {
				memcpy(buf, _buffer + bitIndex / 8, len);
			} else {
				size_t curIndex = bitIndex;
				for (size_t i = 0; i < len; ++i) {
					buf[i] = _getByte(curIndex);
					curIndex += 8;
				}
			}
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator;
		}
	};
}

#endif
