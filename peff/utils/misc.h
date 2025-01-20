#ifndef _PEFF_UTILS_MISC_H_
#define _PEFF_UTILS_MISC_H_

#include "basedefs.h"
#include <peff/base/alloc.h>
#include <type_traits>
#include <cstring>

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

	template <typename T>
	class Uninitialized final {
	private:
		char _buf[sizeof(T)];
		bool _inited;

	public:
		PEFF_FORCEINLINE Uninitialized() {
			_inited = false;
		}
		Uninitialized(const Uninitialized<T> &) = delete;
		PEFF_FORCEINLINE Uninitialized(Uninitialized<T> &&rhs) {
			if ((_inited = rhs._inited)) {
				memmove(_buf, rhs._buf, sizeof(T));
				rhs._inited = false;
			}
		}
		PEFF_FORCEINLINE ~Uninitialized() {
			if(_inited) {
				std::destroy_at<T>((T*)_buf);
			}
		}
		PEFF_FORCEINLINE T &operator*() {
			assert(("The value has not initialized yet", _inited));
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE const T &operator*() const {
			assert(("The value has not initialized yet", _inited));
			return *(T *)_buf;
		}
		PEFF_FORCEINLINE T *operator->() {
			assert(("The value has not initialized yet", _inited));
			return (T *)_buf;
		}
		PEFF_FORCEINLINE const T *operator->() const {
			assert(("The value has not initialized yet", _inited));
			return (T *)_buf;
		}
		PEFF_FORCEINLINE T &&release() {
			assert(("The value has not initialized yet", _inited));
			_inited = false;
			return std::move(*(T *)_buf);
		}
		PEFF_FORCEINLINE T *data() {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE const T *data() const {
			return (T *)_buf;
		}
		PEFF_FORCEINLINE bool inited() const {
			return _inited;
		}
		PEFF_FORCEINLINE bool copyFrom(const T &src) {
			assert(!_inited);
			if (!copy(*(T *)_buf, src)) {
				return false;
			}
			_inited = true;
			return true;
		}
	};
}

#endif
