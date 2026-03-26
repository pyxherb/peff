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
		PEFF_FORCEINLINE BitSet() {
		}
		PEFF_FORCEINLINE ~BitSet() = default;

		PEFF_FORCEINLINE size_t size() const {
			return BYTE_SIZE;
		}

		PEFF_FORCEINLINE size_t bit_size() const {
			return N;
		}

		PEFF_FORCEINLINE uint8_t *data() const {
			return _buffer;
		}

		PEFF_FORCEINLINE void set_bit(size_t bit_index) {
			assert(bit_index < N);
			_set_bit(bit_index);
		}

		PEFF_FORCEINLINE void clear_bit(size_t bit_index) {
			assert(bit_index < N);
			_clear_bit(bit_index);
		}

		PEFF_FORCEINLINE bool get_bit(size_t bit_index) const {
			assert(bit_index < N);
			return (_buffer[(bit_index >> 3)] >> (bit_index & 7)) & 1;
		}

		PEFF_FORCEINLINE void fill_set(size_t bit_index, size_t num_bits) {
			assert(bit_index < N);
			assert(bit_index + num_bits <= N);

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
			assert(bit_index < N);
			assert(bit_index + num_bits < N);

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
			assert(bit_index + 8 <= N);

			_set_byte(bit_index, b, 8);
		}

		PEFF_FORCEINLINE void set_byte(size_t bit_index, uint8_t b, uint8_t num_bits) {
			assert(bit_index + num_bits <= N);

			_set_byte(bit_index, b, num_bits);
		}

		PEFF_FORCEINLINE uint8_t get_byte(size_t bit_index) const {
			assert(bit_index + 8 <= N);

			return _get_byte(bit_index, 8);
		}

		PEFF_FORCEINLINE uint8_t get_byte(size_t bit_index, uint8_t num_bits) const {
			assert(num_bits);
			assert(num_bits <= 8);
			assert(bit_index + num_bits <= N);

			return _get_byte(bit_index, num_bits);
		}

		PEFF_FORCEINLINE void get_bytes(char *buf, size_t len, size_t bit_index) const {
			assert(bit_index + 8 * len <= N);

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
	};
}

#endif
