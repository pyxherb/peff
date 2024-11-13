#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include <peff/base/allocator.h>
#include <array>

namespace peff {
	template <typename T>
	PEFF_FORCEINLINE bool copy(T &out, const T &in) {
		if constexpr (IsCopyable<T>::value) {
			return in.copy(out);
		} else if constexpr (std::is_nothrow_constructible_v<T>) {
			new (&out) T(in);
		} else {
			throw std::logic_error("The type is not copyable");
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
		} else {
			throw std::logic_error("The type is not copy-assignable");
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
}

#endif
