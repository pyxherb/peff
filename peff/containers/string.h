#ifndef _PEFF_CONTAINERS_STRING_H_
#define _PEFF_CONTAINERS_STRING_H_

#include "basedefs.h"
#include <peff/base/scope_guard.h>
#include <peff/utils/hash.h>
#include <peff/base/alloc.h>
#include "dynarray.h"
#include <string_view>

namespace peff {
	class String {
	private:
		using ArrayType = DynArray<char>;
		ArrayType _array;

	public:
		using Iterator = char *;
		using ConstIterator = const char *;

		PEFF_FORCEINLINE String(Alloc *allocator) : _array(allocator) {
		}
		PEFF_FORCEINLINE String(String &&rhs) noexcept : _array(std::move(rhs._array)) {
		}
		PEFF_FORCEINLINE ~String() {}

		PEFF_FORCEINLINE String &operator=(String &&rhs) noexcept {
			verify_allocator(allocator(), rhs.allocator());
			_array = std::move(rhs._array);

			return *this;
		}

		PEFF_FORCEINLINE size_t size() const {
			if (!_array.size())
				return 0;
			return _array.size() - 1;
		}

		PEFF_FORCEINLINE bool resize(size_t length) {
			if (!_array.resize_uninit(length + 1))
				return false;
			_array.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool resize_with(size_t length, const char &filler) {
			const size_t original_length = _array.size();
			if (!_array.resize_uninit(length + 1))
				return false;
			memset(_array.data() + original_length, filler, length - original_length);
			_array.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool resize_and_shrink(size_t length) {
			if (!_array.resize_and_shrink_uninit(length + 1))
				return false;
			_array.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool resize_and_shrinkWith(size_t length, const char &filler) {
			const size_t original_length = _array.size();
			if (!_array.resize_and_shrink_uninit(length + 1))
				return false;
			memset(_array.data() + original_length, filler, length - original_length);
			_array.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE void clear() {
			_array.clear();
		}

		PEFF_FORCEINLINE void clear_and_shrink() {
			_array.clear_and_shrink();
		}

		PEFF_FORCEINLINE char &at(size_t index) {
			return _array.at(index);
		}

		PEFF_FORCEINLINE const char &at(size_t index) const {
			return _array.at(index);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(size_t index, char &&data) {
			return _array.insert(index, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool push_front(char &&data) {
			return _array.push_front(std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool push_back(char &&data) {
			if (size_t index = _array.size(); index) {
				if (!_array.push_back('\0')) {
					return false;
				}
				_array.at(index - 1) = data;
			} else {
				if (!_array.resize(2))
					return false;
				_array.at(0) = data;
				_array.at(1) = '\0';
			}
			return true;
		}

		PEFF_FORCEINLINE void pop_front() {
			_array.pop_front();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pop_front_and_shrink() {
			return _array.pop_front_and_shrink();
		}

		PEFF_FORCEINLINE void pop_back() {
			_array.pop_back();
			_array.back() = '\0';
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pop_back_and_shrink() {
			if (!_array.pop_back_and_shrink())
				return false;
			_array.back() = '\0';
			return true;
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _array.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) noexcept {
			_array.replace_allocator(rhs);
		}

		PEFF_FORCEINLINE char *data() {
			if (_array.size())
				return _array.data();
			return const_cast<char *>("");
		}

		PEFF_FORCEINLINE const char *data() const {
			if (_array.size())
				return _array.data();
			return "";
		}

		PEFF_FORCEINLINE void erase_range(size_t idx_start, size_t idx_end) {
			_array.erase_range(idx_start, idx_end);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool erase_range_and_shrink(size_t idx_start, size_t idx_end) {
			return _array.erase_range_and_shrink(idx_start, idx_end);
		}

		PEFF_FORCEINLINE bool extract_range_and_shrink(size_t idx_start, size_t idx_end) {
			return _array.extract_range_and_shrink(idx_start, idx_end + 1);
		}

		PEFF_FORCEINLINE void extract_range(size_t idx_start, size_t idx_end) {
			_array.extract_range(idx_start, idx_end + 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const char *data, size_t length) {
			const size_t old_size = size();
			if (!resize(old_size + length))
				return false;
			memcpy(_array.data() + old_size, data, length);
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const char *data) {
			const size_t old_size = size();
			const size_t data_length = strlen(data);
			if (!resize(old_size + data_length))
				return false;
			memcpy(_array.data() + old_size, data, data_length);
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(char data) {
			const size_t old_size = size();
			const size_t data_length = 1;
			if (!resize(old_size + data_length))
				return false;
			_array.data()[old_size] = data;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const String &data) {
			const size_t old_size = size();
			if (!resize(old_size + data.size()))
				return false;
			memcpy(_array.data() + old_size, data.data(), data.size());
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const std::string_view &data) {
			const size_t old_size = size();
			if (!resize(old_size + data.size()))
				return false;
			memcpy(_array.data() + old_size, data.data(), data.size());
			_array.data()[old_size + data.size()] = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool build(const std::string_view &src) {
			if (!resize(src.size()))
				return false;
			memcpy(_array.data(), src.data(), src.size());
			_array.data()[src.size()] = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool build_and_shrink(const std::string_view &src) {
			if (!resize_and_shrink(src.size()))
				return false;
			memcpy(_array.data(), src.data(), src.size());
			_array.data()[src.size()] = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool shrink_to_fit() {
			return _array.shrink_to_fit();
		}

		PEFF_FORCEINLINE Iterator begin() {
			return _array.begin();
		}

		PEFF_FORCEINLINE Iterator end() {
			return _array.end();
		}

		PEFF_FORCEINLINE ConstIterator begin() const {
			return _array.begin();
		}

		PEFF_FORCEINLINE ConstIterator end() const {
			return _array.end();
		}

		PEFF_FORCEINLINE operator std::string_view() const {
			if (!size())
				return std::string_view(_array.data(), 0);
			return std::string_view(_array.data(), _array.size() - 1);
		}

		PEFF_FORCEINLINE bool operator==(const std::string_view &rhs) const {
			if (rhs.size() != size())
				return false;
			return !memcmp(_array.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator==(const String &rhs) const {
			if (rhs.size() != size())
				return false;
			return !memcmp(_array.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator!=(const String &rhs) const {
			if (rhs.size() == size())
				return false;
			return memcmp(_array.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator>(const String &rhs) const {
			if (size() > rhs.size())
				return true;
			if (size() < rhs.size())
				return false;
			return memcmp(_array.data(), rhs.data(), size()) > 0;
		}

		PEFF_FORCEINLINE bool operator<(const String &rhs) const {
			if (size() < rhs.size())
				return true;
			if (size() > rhs.size())
				return false;
			return memcmp(_array.data(), rhs.data(), size()) < 0;
		}
	};

	template <>
	struct Hasher<String> {
		Hasher<std::string_view> inner_hasher;
		PEFF_FORCEINLINE std::conditional_t<sizeof(size_t) <= sizeof(uint32_t), uint32_t, uint64_t> operator()(const String &x) const {
			return inner_hasher(x);
		}
	};
}

PEFF_FORCEINLINE bool operator==(const std::string_view &lhs, const peff::String &rhs) noexcept {
	return rhs == lhs;
}

#endif
