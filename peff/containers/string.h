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
		ArrayType _dynArray;

	public:
		using Iterator = char *;
		using ConstIterator = const char *;

		PEFF_FORCEINLINE String(Alloc *allocator) : _dynArray(allocator) {
		}
		PEFF_FORCEINLINE String(String &&rhs) noexcept : _dynArray(std::move(rhs._dynArray)) {
		}
		PEFF_FORCEINLINE ~String() {}

		PEFF_FORCEINLINE String &operator=(String &&rhs) noexcept {
			verifyAlloc(allocator(), rhs.allocator());
			_dynArray = std::move(rhs._dynArray);

			return *this;
		}

		PEFF_FORCEINLINE size_t size() const {
			if (!_dynArray.size())
				return 0;
			return _dynArray.size() - 1;
		}

		PEFF_FORCEINLINE bool resize(size_t length) {
			if (!_dynArray.resizeUninitialized(length + 1))
				return false;
			_dynArray.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool resizeWith(size_t length, const char &filler) {
			const size_t originalLength = _dynArray.size();
			if (!_dynArray.resizeUninitialized(length + 1))
				return false;
			memset(_dynArray.data() + originalLength, filler, length - originalLength);
			_dynArray.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE void clear() {
			_dynArray.clear();
		}

		PEFF_FORCEINLINE char &at(size_t index) {
			return _dynArray.at(index);
		}

		PEFF_FORCEINLINE const char &at(size_t index) const {
			return _dynArray.at(index);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(size_t index, char &&data) {
			return _dynArray.insert(index, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushFront(char &&data) {
			return _dynArray.pushFront(std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushBack(char &&data) {
			if (size_t index = _dynArray.size(); index) {
				if (!_dynArray.pushBack('\0')) {
					return false;
				}
				_dynArray.at(index - 1) = data;
			} else {
				if (!_dynArray.resize(2))
					return false;
				_dynArray.at(0) = data;
				_dynArray.at(1) = '\0';
			}
			return true;
		}

		PEFF_FORCEINLINE void popFront() {
			_dynArray.popFront();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popFrontAndShrink() {
			return _dynArray.popFrontAndShrink();
		}

		PEFF_FORCEINLINE void popBack() {
			_dynArray.popBack();
			_dynArray.back() = '\0';
		}

		[[nodiscard]] PEFF_FORCEINLINE bool popBackAndShrink() {
			if (!_dynArray.popBackAndShrink())
				return false;
			_dynArray.back() = '\0';
			return true;
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _dynArray.allocator();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) noexcept {
			_dynArray.replaceAllocator(rhs);
		}

		PEFF_FORCEINLINE char *data() {
			if (_dynArray.size())
				return _dynArray.data();
			return const_cast<char *>("");
		}

		PEFF_FORCEINLINE const char *data() const {
			if (_dynArray.size())
				return _dynArray.data();
			return "";
		}

		PEFF_FORCEINLINE void eraseRange(size_t idxStart, size_t idxEnd) {
			_dynArray.eraseRange(idxStart, idxEnd);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool eraseRangeAndShrink(size_t idxStart, size_t idxEnd) {
			return _dynArray.eraseRangeAndShrink(idxStart, idxEnd);
		}

		PEFF_FORCEINLINE bool extractRangeAndShrink(size_t idxStart, size_t idxEnd) {
			return _dynArray.extractRangeAndShrink(idxStart, idxEnd + 1);
		}

		PEFF_FORCEINLINE void extractRange(size_t idxStart, size_t idxEnd) {
			_dynArray.extractRange(idxStart, idxEnd + 1);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const char *data, size_t length) {
			const size_t oldSize = size();
			if (!resize(oldSize + length))
				return false;
			memcpy(_dynArray.data() + oldSize, data, length);
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const char *data) {
			const size_t oldSize = size();
			const size_t dataLength = strlen(data);
			if (!resize(oldSize + dataLength))
				return false;
			memcpy(_dynArray.data() + oldSize, data, dataLength);
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(char data) {
			const size_t oldSize = size();
			const size_t dataLength = 1;
			if (!resize(oldSize + dataLength))
				return false;
			_dynArray.data()[oldSize] = data;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const String &data) {
			const size_t oldSize = size();
			if (!resize(oldSize + data.size()))
				return false;
			memcpy(_dynArray.data() + oldSize, data.data(), data.size());
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool append(const std::string_view &data) {
			const size_t oldSize = size();
			if (!resize(oldSize + data.size()))
				return false;
			memcpy(_dynArray.data() + oldSize, data.data(), data.size());
			_dynArray.data()[oldSize + data.size()] = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool build(const std::string_view &src) {
			if (!resize(src.size()))
				return false;
			memcpy(_dynArray.data(), src.data(), src.size());
			_dynArray.data()[src.size()] = '\0';
			return true;
		}

		PEFF_FORCEINLINE Iterator begin() {
			return _dynArray.begin();
		}

		PEFF_FORCEINLINE Iterator end() {
			return _dynArray.end();
		}

		PEFF_FORCEINLINE ConstIterator begin() const {
			return _dynArray.begin();
		}

		PEFF_FORCEINLINE ConstIterator end() const {
			return _dynArray.end();
		}

		PEFF_FORCEINLINE operator std::string_view() const {
			if (!size())
				return std::string_view(_dynArray.data(), 0);
			return std::string_view(_dynArray.data(), _dynArray.size() - 1);
		}

		PEFF_FORCEINLINE bool operator==(const std::string_view &rhs) const {
			if (rhs.size() != size())
				return false;
			return !memcmp(_dynArray.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator==(const String &rhs) const {
			if (rhs.size() != size())
				return false;
			return !memcmp(_dynArray.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator!=(const String &rhs) const {
			if (rhs.size() == size())
				return false;
			return memcmp(_dynArray.data(), rhs.data(), size());
		}

		PEFF_FORCEINLINE bool operator>(const String &rhs) const {
			if (size() > rhs.size())
				return true;
			if (size() < rhs.size())
				return false;
			return memcmp(_dynArray.data(), rhs.data(), size()) > 0;
		}

		PEFF_FORCEINLINE bool operator<(const String &rhs) const {
			if (size() < rhs.size())
				return true;
			if (size() > rhs.size())
				return false;
			return memcmp(_dynArray.data(), rhs.data(), size()) < 0;
		}
	};

	template <>
	struct Hasher<String> {
		PEFF_FORCEINLINE std::conditional_t<sizeof(size_t) <= sizeof(uint32_t), uint32_t, uint64_t> operator()(const String &x) const {
			if constexpr (sizeof(size_t) <= sizeof(uint32_t)) {
				return cityHash32(x.data(), x.size());
			} else {
				return cityHash64(x.data(), x.size());
			}
		}
	};

	template <>
	struct Hasher<std::string_view> {
		PEFF_FORCEINLINE std::conditional_t<sizeof(size_t) <= sizeof(uint32_t), uint32_t, uint64_t> operator()(const std::string_view &x) const {
			if constexpr (sizeof(size_t) <= sizeof(uint32_t)) {
				return cityHash32(x.data(), x.size());
			} else {
				return cityHash64(x.data(), x.size());
			}
		}
	};
}

PEFF_FORCEINLINE bool operator==(const std::string_view &lhs, const peff::String &rhs) noexcept {
	return rhs == lhs;
}

#endif
