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
		size_t _num_bits = 0;
		size_t _capacity = 0;
		uint8_t *_buffer = nullptr;
		peff::RcObjectPtr<Alloc> _allocator;

		PEFF_FORCEINLINE bool _auto_resize_capacity(size_t num_bits) {
			if (num_bits < (_capacity >> 1)) {
				if (!reserve(num_bits)) {
					return false;
				}
			} else if (num_bits > _capacity) {
				if (!reserve(num_bits)) {
					return false;
				}
			}
			return true;
		}

		PEFF_FORCEINLINE bool _resize_or_expand_uninit(size_t num_bits) {
			if (num_bits > _capacity) {
				if (!reserve(num_bits)) {
					return false;
				}
			}
			return true;
		}

		PEFF_FORCEINLINE bool _resize_or_expand(size_t num_bits, bool init_value) {
			if (num_bits > _capacity) {
				if (!reserve(num_bits)) {
					return false;
				}
			} else {
				if (num_bits > _num_bits) {
					if (init_value) {
						fill_set(_num_bits, num_bits - _num_bits);
					} else {
						fill_clear(_num_bits, num_bits - _num_bits);
					}
				}
			}
			return true;
		}

		PEFF_FORCEINLINE void _set_bit(size_t bit_index) {
			_buffer[(bit_index >> 3)] |= (1 << (bit_index & 7));
		}

		PEFF_FORCEINLINE void _clear_bit(size_t bit_index) {
			_buffer[(bit_index >> 3)] &= ~(1 << (bit_index & 7));
		}

		PEFF_FORCEINLINE void _set_byte(size_t bit_index, uint8_t b, uint8_t num_bits) {
			size_t index = (bit_index >> 3), bit_index_in_byte = (bit_index & 7);

			if ((bit_index + (num_bits - 1)) >> 3 == index) {
				// Does not across the byte boundary and inside a single byte.
				// Such as 000|111|00
				uint8_t mask = ~(((1 << num_bits) - 1) << bit_index_in_byte);
				_buffer[index] &= mask;
				_buffer[index] |= b << bit_index_in_byte;
			} else {
				// Acrosses the byte boundary.
				// Such as 000|10010 110|00000
				uint8_t mask = 0xff >> (8 - bit_index_in_byte);
				_buffer[index] &= mask;
				_buffer[index] |= b << bit_index_in_byte;

				uint8_t bits_remaining = num_bits - (8 - bit_index_in_byte);
				mask = 0xff << bits_remaining;
				_buffer[index + 1] &= mask;
				_buffer[index + 1] |= b >> (8 - bit_index_in_byte);
			}

			// Used for verifying if the set byte works correctly
			uint8_t test = _get_byte(bit_index, num_bits);
			if (num_bits < 8) {
				test &= (0xff >> (8 - num_bits));
				b &= (0xff >> (8 - num_bits));
			}
			assert(test == b);
		}

		PEFF_FORCEINLINE uint8_t _get_byte(size_t bit_index, uint8_t num_bits) const {
			if (bit_index & 7) {
				size_t index_in_byte = bit_index >> 3, bit_offset = bit_index & 7;
				if (((bit_index + (num_bits - 1)) >> 3) > index_in_byte) {
					uint8_t l = _buffer[index_in_byte] >> (bit_offset), h = _buffer[index_in_byte + 1] << (8 - bit_offset);
					return (l | h) & (0xff >> (8 - num_bits));
				} else {
					return (_buffer[index_in_byte] >> (bit_offset)) & (0xff >> (8 - num_bits));
				}
			}
			return _buffer[bit_index >> 3] & (0xff >> (8 - num_bits));
		}

	public:
		PEFF_FORCEINLINE BitArray(Alloc *allocator) : _allocator(allocator) {
		}
		PEFF_FORCEINLINE ~BitArray() {
			if (_buffer)
				_allocator->release(_buffer, size(), 1);
		}

		PEFF_FORCEINLINE size_t size() const {
			return (_num_bits + 7) >> 3;
		}

		PEFF_FORCEINLINE size_t bit_size() const {
			return _num_bits;
		}

		PEFF_FORCEINLINE bool reserve(size_t num_bits) {
			if (num_bits) {
				const size_t num_bytes = (num_bits + 7) >> 3;

				uint8_t *new_buffer = (uint8_t *)_allocator->alloc(num_bytes, 1);
				if (!new_buffer)
					return false;

				if (_buffer) {
					if (num_bytes > size()) {
						memcpy((void *)new_buffer, (void *)_buffer, size());
					} else {
						memcpy((void *)new_buffer, (void *)_buffer, num_bytes);
					}
				}
				if (_buffer)
					_allocator->release(_buffer, size(), 1);

				_buffer = new_buffer;
			} else {
				if (_buffer)
					_allocator->release(_buffer, size(), 1);
			}
			_capacity = ((num_bits + 7) >> 3) << 3;

			return true;
		}

		PEFF_FORCEINLINE bool resize_uninit(size_t num_bits) {
			if (!_auto_resize_capacity(num_bits))
				return false;
			_num_bits = num_bits;
			return true;
		}

		PEFF_FORCEINLINE bool push_back(bool bit) {
			if (!_resize_or_expand_uninit(_num_bits + 1)) {
				return false;
			}
			if (bit) {
				_set_bit(_num_bits);
			} else {
				_clear_bit(_num_bits);
			}
			++_num_bits;

			return true;
		}

		PEFF_FORCEINLINE bool push_back_byte(uint8_t b) {
			if (!_resize_or_expand_uninit(_num_bits + 8)) {
				return false;
			}
			_set_byte(_num_bits, b, 8);
			_num_bits += 8;

			return true;
		}

		PEFF_FORCEINLINE bool push_back_bytes(char *data, size_t len) {
			if (!_resize_or_expand_uninit(_num_bits + len * 8)) {
				return false;
			}
			for (size_t i = 0; i < len; ++i) {
				_set_byte(_num_bits, data[i], 8);
				_num_bits += 8;
			}
			return true;
		}

		PEFF_FORCEINLINE void pop_back() {
			--_num_bits;
		}

		PEFF_FORCEINLINE void pop_byte() {
			_num_bits -= 8;
		}

		PEFF_FORCEINLINE bool pop_back_and_resize_capacity() {
			if (!_auto_resize_capacity(_num_bits - 1)) {
				return false;
			}
			--_num_bits;

			return true;
		}

		PEFF_FORCEINLINE bool pop_back_byte_and_resize_capacity() {
			if (!_auto_resize_capacity(_num_bits - 8)) {
				return false;
			}
			_num_bits -= 8;

			return true;
		}

		PEFF_FORCEINLINE uint8_t *data() const {
			return _buffer;
		}

		PEFF_FORCEINLINE void set_bit(size_t bit_index) {
			assert(bit_index < _num_bits);
			_set_bit(bit_index);
		}

		PEFF_FORCEINLINE void clear_bit(size_t bit_index) {
			assert(bit_index < _num_bits);
			_clear_bit(bit_index);
		}

		PEFF_FORCEINLINE bool get_bit(size_t bit_index) const {
			assert(bit_index < _num_bits);
			return (_buffer[(bit_index >> 3)] >> (bit_index & 7)) & 1;
		}

		PEFF_FORCEINLINE void fill_set(size_t bit_index, size_t num_bits) {
			assert(bit_index < _num_bits);
			assert(bit_index + num_bits <= _num_bits);

			size_t idx_end = bit_index + num_bits;

			size_t idx_start_fill_byte = (bit_index + 7) >> 3;
			size_t idx_end_fill_byte = idx_end >> 3;
			size_t sz_fill_byte = idx_end_fill_byte - idx_start_fill_byte;

			if (bit_index & 7) {
				uint8_t start_bits = 0xff << (bit_index & 7);
				_buffer[bit_index >> 3] |= start_bits;
			}

			if (sz_fill_byte)
				memset(_buffer + idx_start_fill_byte, 0xff, sz_fill_byte);

			if (idx_end & 7) {
				uint8_t end_bits = 0xff >> (7 - (bit_index & 7));
				_buffer[idx_end >> 3] |= end_bits;
			}
		}

		PEFF_FORCEINLINE void fill_clear(size_t bit_index, size_t num_bits) {
			assert(bit_index < _num_bits);
			assert(bit_index + num_bits < _num_bits);

			size_t idx_end = bit_index + num_bits;

			size_t idx_start_fill_byte = (bit_index + 7) >> 3;
			size_t idx_end_fill_byte = idx_end >> 3;
			size_t sz_fill_byte = idx_end_fill_byte - idx_start_fill_byte;

			if (bit_index & 7) {
				uint8_t start_bits = 0xff >> (7 - (bit_index & 7));
				_buffer[bit_index >> 3] &= start_bits;
			}

			if (sz_fill_byte)
				memset(_buffer + idx_start_fill_byte, 0x00, sz_fill_byte);

			if (idx_end & 7) {
				uint8_t end_bits = 0xff << (bit_index & 7);
				_buffer[idx_end >> 3] &= end_bits;
			}
		}

		PEFF_FORCEINLINE void set_byte(size_t bit_index, uint8_t b) {
			assert(bit_index + 8 <= _num_bits);

			_set_byte(bit_index, b, 8);
		}

		PEFF_FORCEINLINE void set_byte(size_t bit_index, uint8_t b, uint8_t num_bits) {
			assert(bit_index + num_bits <= _num_bits);

			_set_byte(bit_index, b, num_bits);
		}

		PEFF_FORCEINLINE uint8_t get_byte(size_t bit_index) const {
			assert(bit_index + 8 <= _num_bits);

			return _get_byte(bit_index, 8);
		}

		PEFF_FORCEINLINE uint8_t get_byte(size_t bit_index, uint8_t num_bits) const {
			assert(num_bits);
			assert(num_bits <= 8);
			assert(bit_index + num_bits <= _num_bits);

			return _get_byte(bit_index, num_bits);
		}

		PEFF_FORCEINLINE void get_bytes(char *buf, size_t len, size_t bit_index) const {
			assert(bit_index + 8 * len <= _num_bits);

			if (!((bit_index) & 7)) {
				memcpy(buf, _buffer + bit_index / 8, len);
			} else {
				size_t cur_index = bit_index;
				for (size_t i = 0; i < len; ++i) {
					buf[i] = _get_byte(cur_index, 8);
					cur_index += 8;
				}
			}
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator.get();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) {
			verify_replaceable(_allocator.get(), rhs);

			_allocator = rhs;
		}
	};
}

#endif
