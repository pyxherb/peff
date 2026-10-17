#ifndef _PEFF_CONTAINERS_BITARRAY_H_
#define _PEFF_CONTAINERS_BITARRAY_H_

#include "dynarray.h"
#include <peff/base/alloc.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

namespace peff {
	class BitArray {
	private:
		peff::DynArray<uint8_t> _buffer;
		size_t _num_bits = 0;

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

				uint8_t bits_remaining = static_cast<uint8_t>(num_bits - (8 - bit_index_in_byte));
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
		PEFF_FORCEINLINE BitArray(PEFF_NONNULL Alloc *allocator) noexcept : _buffer(allocator) {
		}
		PEFF_FORCEINLINE BitArray(BitArray &&rhs) noexcept : _buffer(std::move(rhs._buffer)), _num_bits(rhs._num_bits) {
			rhs._num_bits = 0;
		}
		PEFF_FORCEINLINE ~BitArray() {
		}

		PEFF_FORCEINLINE BitArray& operator=(BitArray &&rhs) noexcept {
			_buffer = std::move(rhs._buffer);
			_num_bits = rhs._num_bits;
			rhs._num_bits = 0;
			return *this;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _num_bits;
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return !_num_bits;
		}

		PEFF_FORCEINLINE size_t size_bytes() const {
			return (_num_bits + 7) >> 3;
		}

		PEFF_FORCEINLINE bool resize_uninit(size_t num_bits) {
			if (!_buffer.resize_uninit((num_bits + 7) / 8))
				return false;
			_num_bits = num_bits;
			return true;
		}

		PEFF_FORCEINLINE bool resize(size_t num_bits) {
			if (!_buffer.resize_uninit((num_bits + 7) / 8))
				return false;
			memset(_buffer.data(), 0, (num_bits + 7) / 8);
			_num_bits = num_bits;
			return true;
		}

		PEFF_FORCEINLINE bool push_back(bool bit) {
			size_t old_num_bits = _num_bits;
			if (!resize_uninit(_num_bits + 1)) {
				return false;
			}
			if (bit) {
				_set_bit(old_num_bits);
			} else {
				_clear_bit(old_num_bits);
			}

			return true;
		}

		PEFF_FORCEINLINE bool push_back_byte(uint8_t b) {
			size_t old_num_bits = _num_bits;
			if (!resize_uninit(_num_bits + 8)) {
				return false;
			}
			_set_byte(old_num_bits, b, 8);

			return true;
		}

		PEFF_FORCEINLINE bool push_back_bytes(PEFF_IN const char *data, size_t len) {
			size_t old_num_bits = _num_bits;
			if (!resize_uninit(_num_bits + len * 8)) {
				return false;
			}
			for (size_t i = 0; i < len; ++i) {
				_set_byte(old_num_bits + i * 8, data[i], 8);
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
			if (!resize_uninit(_num_bits - 1)) {
				return false;
			}

			return true;
		}

		PEFF_FORCEINLINE bool pop_back_byte_and_resize_capacity() {
			// resize_uninit() already updates the number of bits.
			return resize_uninit(_num_bits - 8);
		}

		PEFF_FORCEINLINE PEFF_NULLABLE const uint8_t *data() const {
			return _buffer.data();
		}

		PEFF_FORCEINLINE PEFF_NULLABLE uint8_t *data() {
			return _buffer.data();
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

			if (!num_bits)
				return;

			const size_t idx_end = bit_index + num_bits;
			const size_t first_byte = bit_index >> 3;
			const size_t last_byte = (idx_end - 1) >> 3;

			if (first_byte == last_byte) {
				// The whole range lives inside a single byte.
				_buffer[first_byte] |= (uint8_t)((0xffu << (bit_index & 7)) & (0xffu >> (7 - ((idx_end - 1) & 7))));
				return;
			}

			if (bit_index & 7)
				_buffer[first_byte] |= (uint8_t)(0xffu << (bit_index & 7));

			if (last_byte > first_byte + 1)
				memset(_buffer.data() + first_byte + 1, 0xff, last_byte - first_byte - 1);

			_buffer[last_byte] |= (uint8_t)(0xffu >> (7 - ((idx_end - 1) & 7)));
		}

		PEFF_FORCEINLINE void fill_clear(size_t bit_index, size_t num_bits) {
			assert(bit_index < _num_bits);
			assert(bit_index + num_bits <= _num_bits);

			if (!num_bits)
				return;

			const size_t idx_end = bit_index + num_bits;
			const size_t first_byte = bit_index >> 3;
			const size_t last_byte = (idx_end - 1) >> 3;

			if (first_byte == last_byte) {
				// The whole range lives inside a single byte.
				_buffer[first_byte] &= (uint8_t) ~((0xffu << (bit_index & 7)) & (0xffu >> (7 - ((idx_end - 1) & 7))));
				return;
			}

			if (bit_index & 7)
				_buffer[first_byte] &= (uint8_t) ~(0xffu << (bit_index & 7));

			if (last_byte > first_byte + 1)
				memset(_buffer.data() + first_byte + 1, 0x00, last_byte - first_byte - 1);

			_buffer[last_byte] &= (uint8_t) ~(0xffu >> (7 - ((idx_end - 1) & 7)));
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

		PEFF_FORCEINLINE void get_bytes(PEFF_OUT_ARRAY(len) char *buf, size_t len, size_t bit_index) const {
			assert(bit_index + 8 * len <= _num_bits);

			if (!((bit_index) & 7)) {
				memcpy(buf, _buffer.data() + bit_index / 8, len);
			} else {
				size_t cur_index = bit_index;
				for (size_t i = 0; i < len; ++i) {
					buf[i] = _get_byte(cur_index, 8);
					cur_index += 8;
				}
			}
		}

		PEFF_FORCEINLINE PEFF_NONNULL Alloc *allocator() const {
			return _buffer.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) {
			_buffer.replace_allocator(rhs);
		}
	};
}

#endif
