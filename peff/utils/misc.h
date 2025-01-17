#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <type_traits>

namespace peff {
	template <typename T>
	PEFF_FORCEINLINE bool copy(T &out, const T &in) {
		if constexpr (IsCopyable<T>::value) {
			return in.copy(out);
		} else if constexpr (std::is_nothrow_constructible_v<T>) {
			constructAt<T>(&out, in);
			return true;
		} else {
			assert(("The type is not copyable", false));
			return false;
		}
	}

	PEFF_FORCEINLINE bool copy(int &out, const int &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(unsigned int &out, const unsigned int &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(long &out, const long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(unsigned long &out, const unsigned long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(long long &out, const long long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(unsigned long long &out, const unsigned long long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(short &out, const short &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(unsigned short &out, const unsigned short &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(char &out, const char &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(unsigned char &out, const unsigned char &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(float &out, const float &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copy(double &out, const double &in) {
		out = in;
		return true;
	}

	template <typename T>
	PEFF_FORCEINLINE bool copyAssign(T &out, const T &in) {
		if constexpr (IsCopyable<T>::value) {
			return in.copyAssign(out);
		} else if constexpr (std::is_nothrow_copy_assignable_v<T>) {
			out = in;
			return true;
		} else {
			static_assert(!std::is_same_v<T, T>, "The type is not copy-assignable");
		}
	}

	PEFF_FORCEINLINE bool copyAssign(int &out, const int &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(unsigned int &out, const unsigned int &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(long &out, const long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(unsigned long &out, const unsigned long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(long long &out, const long long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(unsigned long long &out, const unsigned long long &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(short &out, const short &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(unsigned short &out, const unsigned short &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(char &out, const char &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(unsigned char &out, const unsigned char &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(float &out, const float &in) {
		out = in;
		return true;
	}

	PEFF_FORCEINLINE bool copyAssign(double &out, const double &in) {
		out = in;
		return true;
	}

	enum class Endian : uint8_t {
		Little = 0,
		Big
	};

	PEFF_UTILS_API Endian testNativeEndian();
}

#endif
