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
#include <iostream>
#include <string>

enum class LZWhatDataType : uint8_t {
	RawByte,
	Replication
};

bool lzwhat_encode_varint64(peff::BitArray &bit_array, uint64_t data) {
	size_t beginning = bit_array.bit_size();
	if (data < (1llu << 7)) {
		uint8_t b = 0b1;

		if (!bit_array.resize_uninit(beginning + 1 + 7)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 1);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 1, a, 7);
	} else if (data < (1llu << 14)) {
		uint8_t b = 0b01;

		if (!bit_array.resize_uninit(beginning + 2 + 14)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 2);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 2, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 2 + 7, a, 7);
	} else if (data < (1llu << 21)) {
		uint8_t b = 0b001;

		if (!bit_array.resize_uninit(beginning + 3 + 21)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 3);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 3, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 3 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 3 + 14, a, 7);
	} else if (data < (1llu << 28)) {
		uint8_t b = 0b0001;

		if (!bit_array.resize_uninit(beginning + 4 + 28)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 4);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 4, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 4 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 4 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 4 + 21, a, 7);
	} else if (data < (1llu << 35)) {
		uint8_t b = 0b00001;

		if (!bit_array.resize_uninit(beginning + 5 + 35)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 5);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 5, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 5 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 5 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 5 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bit_array.set_byte(beginning + 5 + 28, a, 7);
	} else if (data < (1llu << 42)) {
		uint8_t b = 0b000001;

		if (!bit_array.resize_uninit(beginning + 6 + 42)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 6);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 6, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 6 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 6 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 6 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bit_array.set_byte(beginning + 6 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bit_array.set_byte(beginning + 6 + 35, a, 7);
	} else if (data < (1llu << 49)) {
		uint8_t b = 0b0000001;

		if (!bit_array.resize_uninit(beginning + 7 + 49)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 7);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 7, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 7 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 7 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 7 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bit_array.set_byte(beginning + 7 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bit_array.set_byte(beginning + 7 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bit_array.set_byte(beginning, a, 7);
	} else if (data < (1llu << 56)) {
		uint8_t b = 0b00000001;

		if (!bit_array.resize_uninit(beginning + 8 + 56)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 8);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 8, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 42, a, 7);
		a = (data >> 49) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 49, a, 7);
	} else {
		uint8_t b = 0b00000000;

		if (!bit_array.resize_uninit(beginning + 9 + 64)) {
			return false;
		}
		bit_array.set_byte(beginning, b, 8);

		uint8_t a;
		a = data & 0b1111111;
		bit_array.set_byte(beginning + 8, a, 7);
		a = (data >> 7) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 7, a, 7);
		a = (data >> 14) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 14, a, 7);
		a = (data >> 21) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 21, a, 7);
		a = (data >> 28) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 28, a, 7);
		a = (data >> 35) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 35, a, 7);
		a = (data >> 42) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 42, a, 7);
		a = (data >> 49) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 49, a, 7);
		a = (data >> 56) & 0b1111111;
		bit_array.set_byte(beginning + 8 + 56, a, 8);
	}

	return true;
}

bool lzwhat_decode_varint64(const peff::BitArray &bit_array, size_t &cur_index, uint64_t &data_out) {
	data_out = 0;

	auto read_first_bits = [](const peff::BitArray &bit_array, size_t &cur_index, size_t &data_out) -> bool {
		if (cur_index + 7 > bit_array.bit_size()) {
			return false;
		}
		uint8_t a;
		a = bit_array.get_byte(cur_index, 7);
		data_out = a;
		cur_index += 7;
		return true;
	};
	auto shift_and_read = [](const peff::BitArray &bit_array, size_t &cur_index, size_t &data_out) -> bool {
		if (cur_index + 7 > bit_array.bit_size()) {
			return false;
		}
		uint8_t a;
		a = bit_array.get_byte(cur_index, 7);
		data_out <<= 7;
		data_out |= a;
		cur_index += 7;
		return true;
	};
	auto read_last_bits = [](const peff::BitArray &bit_array, size_t &cur_index, size_t &data_out) -> bool {
		if (cur_index + 8 > bit_array.bit_size()) {
			return false;
		}
		uint8_t a;
		a = bit_array.get_byte(cur_index, 8);
		data_out <<= 7;
		data_out = a;
		cur_index += 8;
		return true;
	};

	if (cur_index + 1 > bit_array.bit_size()) {
		return false;
	}
	if (!bit_array.get_bit(cur_index++)) {
		if (cur_index + 1 > bit_array.bit_size()) {
			return false;
		}
		if (!bit_array.get_bit(cur_index++)) {
			if (cur_index + 1 > bit_array.bit_size()) {
				return false;
			}
			if (!bit_array.get_bit(cur_index++)) {
				if (cur_index + 1 > bit_array.bit_size()) {
					return false;
				}
				if (!bit_array.get_bit(cur_index++)) {
					if (cur_index + 1 > bit_array.bit_size()) {
						return false;
					}
					if (!bit_array.get_bit(cur_index++)) {
						if (cur_index + 1 > bit_array.bit_size()) {
							return false;
						}
						if (!bit_array.get_bit(cur_index++)) {
							if (cur_index + 1 > bit_array.bit_size()) {
								return false;
							}
							if (!bit_array.get_bit(cur_index++)) {
								if (cur_index + 1 > bit_array.bit_size()) {
									return false;
								}
								if (!bit_array.get_bit(cur_index++)) {
									// 64-bit
									if (!read_first_bits(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!read_last_bits(bit_array, cur_index, data_out))
										return false;
								} else {
									// 56-bit
									if (!read_first_bits(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
									if (!shift_and_read(bit_array, cur_index, data_out))
										return false;
								}
							} else {
								// 49-bit
								if (!read_first_bits(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
								if (!shift_and_read(bit_array, cur_index, data_out))
									return false;
							}
						} else {
							// 42-bit
							if (!read_first_bits(bit_array, cur_index, data_out))
								return false;
							if (!shift_and_read(bit_array, cur_index, data_out))
								return false;
							if (!shift_and_read(bit_array, cur_index, data_out))
								return false;
							if (!shift_and_read(bit_array, cur_index, data_out))
								return false;
							if (!shift_and_read(bit_array, cur_index, data_out))
								return false;
							if (!shift_and_read(bit_array, cur_index, data_out))
								return false;
						}
					} else {
						// 35-bit
						if (!read_first_bits(bit_array, cur_index, data_out))
							return false;
						if (!shift_and_read(bit_array, cur_index, data_out))
							return false;
						if (!shift_and_read(bit_array, cur_index, data_out))
							return false;
						if (!shift_and_read(bit_array, cur_index, data_out))
							return false;
						if (!shift_and_read(bit_array, cur_index, data_out))
							return false;
					}
				} else {
					// 28-bit
					if (!read_first_bits(bit_array, cur_index, data_out))
						return false;
					if (!shift_and_read(bit_array, cur_index, data_out))
						return false;
					if (!shift_and_read(bit_array, cur_index, data_out))
						return false;
					if (!shift_and_read(bit_array, cur_index, data_out))
						return false;
				}
			} else {
				// 21-bit
				if (!read_first_bits(bit_array, cur_index, data_out))
					return false;
				if (!shift_and_read(bit_array, cur_index, data_out))
					return false;
				if (!shift_and_read(bit_array, cur_index, data_out))
					return false;
			}
		} else {
			// 14-bit
			if (!read_first_bits(bit_array, cur_index, data_out))
				return false;
			if (!shift_and_read(bit_array, cur_index, data_out))
				return false;
		}
	} else {
		// 7-bit
		if (!read_first_bits(bit_array, cur_index, data_out))
			return false;
	}

	return true;
}

bool lzwhat_compress(peff::BitArray &bit_array, const char *buf, size_t size, size_t dict_size, size_t encode_buffer_size) {
	const char *const buf_limit = buf + size;

	for (size_t i = 0; i < size; ++i) {
		const char *search_buf_ptr,
			*search_end_ptr;

		if (i < dict_size) {
			search_buf_ptr = buf;
		} else {
			search_buf_ptr = buf + i - dict_size;
		}
		search_end_ptr = buf + i;

		const char *match_buf_ptr = search_end_ptr,
				   *match_end_ptr = match_buf_ptr + encode_buffer_size;
		if (match_end_ptr > buf_limit) {
			match_end_ptr = buf_limit;
		}
		size_t search_buf_size = search_end_ptr - search_buf_ptr,
			   match_buf_size = match_end_ptr - match_buf_ptr;

		for (size_t match_len = search_buf_size < match_buf_size ? search_buf_size : match_buf_size; match_len > 0; --match_len) {
			for (size_t off_start = 0; off_start < search_buf_size - match_len; ++off_start) {
				if (!memcmp(search_buf_ptr + off_start, match_buf_ptr, match_len)) {
					size_t disp = (search_end_ptr - (search_buf_ptr + off_start));
					size_t len = match_len;
					printf("(%zu, %zu)\n", disp, len);
					if (!bit_array.push_back(false))
						return false;
					if (!lzwhat_encode_varint64(bit_array, disp)) {
						return false;
					}
					if (!lzwhat_encode_varint64(bit_array, len)) {
						return false;
					}
					/* if (!bit_array.push_back_bytes((char *)&disp, sizeof(disp)))
						return false;
					if (!bit_array.push_back_bytes((char *)&len, sizeof(len)))
						return false;*/
					i += len - 1;
					goto next;
				}
			}
		}

		if (!bit_array.push_back(true))
			return false;
		if (!bit_array.push_back_byte(*(uint8_t *)match_buf_ptr))
			return false;
		printf("'%c'\n", *(uint8_t *)match_buf_ptr);
	next:;
	}

	return true;
}

bool lzwhat_decompress(const peff::BitArray &bit_array, peff::Alloc *allocator, size_t dict_size, peff::DynArray<char> &data_out) {
	size_t i = 0;

	peff::DynArray<char> dict(allocator);

	if (!dict.resize(dict_size)) {
		return false;
	}

	size_t cur_dict_size = 0, cur_dict_index = 0;

	auto calc_dict_index = [](size_t &cur_dict_index, size_t delta, size_t dict_size) {
		assert(delta < dict_size);
		if (cur_dict_index + delta >= dict_size) {
			cur_dict_index = (cur_dict_index + delta) - dict_size;
		} else {
			cur_dict_index += delta;
		}
		assert(cur_dict_index < dict_size);
	};
	auto copy_to_dict = [](char *dict, size_t dict_size, size_t cur_dict_index, const char *in, size_t len) {
		if (cur_dict_index + len > dict_size) {
			memcpy(dict + cur_dict_index, in, dict_size - cur_dict_index);
			memcpy(dict, in + (dict_size - cur_dict_index), cur_dict_index + len - dict_size);
		} else {
			memcpy(dict + cur_dict_index, in, len);
		}
	};
	auto copy_from_dict = [](char *dict, size_t dict_size, size_t cur_dict_index, char *out, size_t len) {
		if (cur_dict_index + len > dict_size) {
			memcpy(out, dict + cur_dict_index, dict_size - cur_dict_index);
			memcpy(out + (dict_size - cur_dict_index), dict, cur_dict_index + len - dict_size);
		} else {
			memcpy(out, dict + cur_dict_index, len);
		}
	};

#define INC_CUR_PTR(i, size, bit_array) \
	if (((i) += (size)) > (bit_array).bit_size()) return false;

	while (i < bit_array.bit_size()) {
		bool b = bit_array.get_bit(i);
		INC_CUR_PTR(i, 1, bit_array);

		if (b) {
			uint8_t c = bit_array.get_byte(i);
			if (!data_out.push_back(+*(char *)&c)) {
				return false;
			}
			INC_CUR_PTR(i, 8, bit_array);
			copy_to_dict(dict.data(), dict_size, cur_dict_index, (char *)&c, 1);
			calc_dict_index(cur_dict_index, 1, dict_size);
		} else {
			size_t start = data_out.size();
			size_t disp;
			size_t len;
			if (!lzwhat_decode_varint64(bit_array, i, disp)) {
				return false;
			}
			/* bit_array.get_bytes((char *)&disp, sizeof(disp), i);*/
			assert(disp <= dict_size);
			/* bit_array.get_bytes((char *)&len, sizeof(len), i);*/
			if (!lzwhat_decode_varint64(bit_array, i, len)) {
				return false;
			}
			assert(len <= dict_size);
			if (!data_out.resize_uninit(data_out.size() + len)) {
				return false;
			}
			size_t left_limit = cur_dict_index;
			calc_dict_index(left_limit, dict_size - disp, dict_size);
			copy_from_dict(dict.data(), dict_size, left_limit, data_out.data() + start, len);
			copy_to_dict(dict.data(), dict_size, cur_dict_index, data_out.data() + start, len);
			calc_dict_index(cur_dict_index, len, dict_size);
		}
	}

	return true;
}

int main() {
	const char s[] = "ABACBAABCBCCA";

	peff::BitArray bit_array(peff::default_allocator());

	if (!lzwhat_compress(bit_array, s, sizeof(s) - 1, 7, 4)) {
		std::terminate();
	}

	peff::DynArray<char> data(peff::default_allocator());

	if (!lzwhat_decompress(bit_array, peff::default_allocator(), 7, data)) {
		std::terminate();
	}

	for (size_t i = 0; i < data.size(); ++i) {
		putchar(data.at(i));
	}

	return 0;
}
