#ifndef _PEFF_CONTAINERS_STRING_H_
#define _PEFF_CONTAINERS_STRING_H_

#include "basedefs.h"
#include <peff/base/scope_guard.h>
#include <peff/utils/hash.h>
#include <peff/base/alloc.h>
#include "dynarray.h"

namespace peff {
	class String {
	private:
		DynArray<char> _dynArray;

	public:
		PEFF_FORCEINLINE String(Alloc *allocator = getDefaultAlloc()) : _dynArray(allocator) {
			clear();
		}
		PEFF_FORCEINLINE String(String &&rhs) noexcept : _dynArray(std::move(rhs._dynArray)) {
		}
		PEFF_FORCEINLINE ~String() {}

		String &operator=(String &&rhs) noexcept {
			_dynArray = std::move(rhs._dynArray);

			return *this;
		}

		PEFF_FORCEINLINE bool copy(String &dest) const {
			new (&dest) String(_dynArray.allocator());

			DynArray<char> arrayCopy;
			if (!(dest._dynArray.copy(arrayCopy))) {
				return false;
			}

			dest._dynArray = std::move(arrayCopy);

			return true;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _dynArray.size() - 1;
		}

		PEFF_FORCEINLINE bool resize(size_t length) {
			if (!_dynArray.resize(length + 1))
				return false;
			_dynArray.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE bool resizeWith(size_t length, const char &filler) {
			if (!_dynArray.resizeWith(length + 1, filler))
				return false;
			_dynArray.at(length) = '\0';
			return true;
		}

		PEFF_FORCEINLINE void clear() {
			_dynArray.resizeWith(1, '\0');
		}

		PEFF_FORCEINLINE char& at(size_t index) {
			return _dynArray.at(index);
		}

		PEFF_FORCEINLINE const char &at(size_t index) const {
			return _dynArray.at(index);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool reserveSlots(size_t index, size_t length) {
			return _dynArray.reserveSlots(index, length);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(size_t index, char &&data) {
			return _dynArray.insert(index, std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushFront(char &&data) {
			return _dynArray.pushFront(std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool pushBack(char &&data) {
			return _dynArray.pushBack(std::move(data));
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _dynArray.allocator();
		}

		PEFF_FORCEINLINE char *data() {
			return _dynArray.data();
		}

		PEFF_FORCEINLINE const char *data() const {
			return _dynArray.data();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool eraseRange(size_t idxStart, size_t idxEnd) {
			return _dynArray.eraseRange(idxStart, idxEnd);
		}

		PEFF_FORCEINLINE void extractRange(size_t idxStart, size_t idxEnd) {
			_dynArray.extractRange(idxStart, idxEnd + 1);
		}
	};

	template <>
	struct Hasher<String> {
		PEFF_FORCEINLINE uint64_t operator()(const String &x) const {
			return djbHash64(x.data(), x.size());
		}
	};
}

PEFF_CONTAINERS_API bool operator==(const peff::String &lhs, const peff::String &rhs) noexcept;

#endif
