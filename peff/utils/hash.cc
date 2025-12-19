#include "hash.h"
#include "byteord.h"
#include "bitops.h"

using namespace peff;

PEFF_UTILS_API uint32_t peff::djbHash32(const char* data, size_t size) {
	uint32_t hash = 5381;
	for (size_t i = 0; i < size; ++i) {
		hash += (hash << 5) + *(data++);
	}
	return hash;
}

PEFF_UTILS_API uint64_t peff::djbHash64(const char *data, size_t size) {
	uint64_t hash = 5381;
	for (size_t i = 0; i < size; ++i) {
		hash += (hash << 5) + *(data++);
	}
	return hash;
}

//
// CityHash - original version by Google, with minor improvements.
// Copyright (C) 2011 Google, Inc.
// Copyright (C) 2025 PEFF Project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
constexpr static const uint32_t CITYHASH_C1 = 0xcc9e2d51;
constexpr static const uint32_t CITYHASH_C2 = 0x1b873593;

PEFF_FORCEINLINE uint64_t _cityHashFetch64(const char *p) {
	if (((uintptr_t)p) & 0x07) {
		return peff::getByteOrder() ? *p : peff::swapByteOrder(*(uint64_t *)p);
	}
	uint64_t data;
	memcpy(&data, p, sizeof(data));
	return data;
}

PEFF_FORCEINLINE uint32_t _cityHashFetch32(const char *p) {
	if (((uintptr_t)p) & 0x03) {
		return peff::getByteOrder() ? *p : peff::swapByteOrder(*(uint32_t *)p);
	}
	uint32_t data;
	memcpy(&data, p, sizeof(data));
	return data;
}

static uint32_t _cityHashFMix(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

static uint32_t _cityHashMur(uint32_t a, uint32_t h) {
	// Helper from Murmur3 for combining two 32-bit values.
	a *= CITYHASH_C1;
	a = peff::rRot(a, 17);
	a *= CITYHASH_C2;
	h ^= a;
	h = peff::rRot(h, 19);
	return h * 5 + 0xe6546b64;
}

static uint32_t _cityHashHash32Len13to24(const char *s, size_t len) {
	uint32_t a = _cityHashFetch32(s - 4 + (len >> 1));
	uint32_t b = _cityHashFetch32(s + 4);
	uint32_t c = _cityHashFetch32(s + len - 8);
	uint32_t d = _cityHashFetch32(s + (len >> 1));
	uint32_t e = _cityHashFetch32(s);
	uint32_t f = _cityHashFetch32(s + len - 4);
	uint32_t h = static_cast<uint32_t>(len);

	return _cityHashFMix(_cityHashMur(f, _cityHashMur(e, _cityHashMur(d, _cityHashMur(c, _cityHashMur(b, _cityHashMur(a, h)))))));
}

static uint32_t _cityHashHash32Len0to4(const char *s, size_t len) {
	uint32_t b = 0;
	uint32_t c = 9;
	for (size_t i = 0; i < len; i++) {
		signed char v = static_cast<signed char>(s[i]);
		b = b * CITYHASH_C1 + static_cast<uint32_t>(v);
		c ^= b;
	}
	return _cityHashFMix(_cityHashMur(b, _cityHashMur(static_cast<uint32_t>(len), c)));
}

static uint32_t _cityHashHash32Len5to12(const char *s, size_t len) {
	uint32_t a = static_cast<uint32_t>(len), b = a * 5, c = 9, d = b;
	a += _cityHashFetch32(s);
	b += _cityHashFetch32(s + len - 4);
	c += _cityHashFetch32(s + ((len >> 1) & 4));
	return _cityHashFMix(_cityHashMur(c, _cityHashMur(b, _cityHashMur(a, d))));
}

#define CITYHASH32_PERMUTE3(a, b, c) \
	std::swap(a, b);                 \
	std::swap(a, c)

PEFF_UTILS_API uint32_t peff::cityHash32(const char *s, size_t len) {
	if (len <= 24) {
		return len <= 12 ? (len <= 4 ? _cityHashHash32Len0to4(s, len) : _cityHashHash32Len5to12(s, len)) : _cityHashHash32Len13to24(s, len);
	}

	uint32_t h = static_cast<uint32_t>(len), g = CITYHASH_C1 * h, f = g;
	uint32_t a0 = peff::rRot(_cityHashFetch32(s + len - 4) * CITYHASH_C1, 17) * CITYHASH_C2;
	uint32_t a1 = peff::rRot(_cityHashFetch32(s + len - 8) * CITYHASH_C1, 17) * CITYHASH_C2;
	uint32_t a2 = peff::rRot(_cityHashFetch32(s + len - 16) * CITYHASH_C1, 17) * CITYHASH_C2;
	uint32_t a3 = peff::rRot(_cityHashFetch32(s + len - 12) * CITYHASH_C1, 17) * CITYHASH_C2;
	uint32_t a4 = peff::rRot(_cityHashFetch32(s + len - 20) * CITYHASH_C1, 17) * CITYHASH_C2;
	h ^= a0;
	h = peff::rRot(h, 19);
	h = h * 5 + 0xe6546b64;
	h ^= a2;
	h = peff::rRot(h, 19);
	h = h * 5 + 0xe6546b64;
	g ^= a1;
	g = peff::rRot(g, 19);
	g = g * 5 + 0xe6546b64;
	g ^= a3;
	g = peff::rRot(g, 19);
	g = g * 5 + 0xe6546b64;
	f += a4;
	f = peff::rRot(f, 19);
	f = f * 5 + 0xe6546b64;
	size_t iters = (len - 1) / 20;
	do {
		uint32_t a0 = peff::rRot(_cityHashFetch32(s) * CITYHASH_C1, 17) * CITYHASH_C2;
		uint32_t a1 = _cityHashFetch32(s + 4);
		uint32_t a2 = peff::rRot(_cityHashFetch32(s + 8) * CITYHASH_C1, 17) * CITYHASH_C2;
		uint32_t a3 = peff::rRot(_cityHashFetch32(s + 12) * CITYHASH_C1, 17) * CITYHASH_C2;
		uint32_t a4 = _cityHashFetch32(s + 16);
		h ^= a0;
		h = peff::rRot(h, 18);
		h = h * 5 + 0xe6546b64;
		f += a1;
		f = peff::rRot(f, 19);
		f = f * CITYHASH_C1;
		g += a2;
		g = peff::rRot(g, 18);
		g = g * 5 + 0xe6546b64;
		h ^= a3 + a1;
		h = peff::rRot(h, 19);
		h = h * 5 + 0xe6546b64;
		g ^= a4;
		g = peff::swapByteOrder(g) * 5;
		h += a4 * 5;
		h = peff::swapByteOrder(h);
		f += a0;
		CITYHASH32_PERMUTE3(f, h, g);
		s += 20;
	} while (--iters != 0);
	g = peff::rRot(g, 11) * CITYHASH_C1;
	g = peff::rRot(g, 17) * CITYHASH_C1;
	f = peff::rRot(f, 11) * CITYHASH_C1;
	f = peff::rRot(f, 17) * CITYHASH_C1;
	h = peff::rRot(h + g, 19);
	h = h * 5 + 0xe6546b64;
	h = peff::rRot(h, 17) * CITYHASH_C1;
	h = peff::rRot(h + f, 19);
	h = h * 5 + 0xe6546b64;
	h = peff::rRot(h, 17) * CITYHASH_C1;
	return h;
}

static uint64_t _cityHashShiftMix(uint64_t val) {
	return val ^ (val >> 47);
}

typedef std::pair<uint64_t, uint64_t> CityHashU128;

constexpr static uint64_t k0 = 0xc3a5c85c97cb3127ULL;
constexpr static uint64_t k1 = 0xb492b66fbe98f273ULL;
constexpr static uint64_t k2 = 0x9ae16a3b2f90404fULL;

PEFF_FORCEINLINE uint64_t _cityHashU128Low64(const CityHashU128 &x) { return x.first; }
PEFF_FORCEINLINE uint64_t _cityHashU128High64(const CityHashU128 &x) { return x.second; }

PEFF_FORCEINLINE uint64_t _cityHashHash128to64(const CityHashU128 &x) {
	// Murmur-inspired hashing.
	const uint64_t kMul = 0x9ddfea08eb382d69ULL;
	uint64_t a = (_cityHashU128Low64(x) ^ _cityHashU128High64(x)) * kMul;
	a ^= (a >> 47);
	uint64_t b = (_cityHashU128High64(x) ^ a) * kMul;
	b ^= (b >> 47);
	b *= kMul;
	return b;
}

static uint64_t _cityHashHashLen16(uint64_t u, uint64_t v) {
	return _cityHashHash128to64(CityHashU128(u, v));
}

static uint64_t _cityHashHashLen16(uint64_t u, uint64_t v, uint64_t mul) {
	uint64_t a = (u ^ v) * mul;
	a ^= (a >> 47);
	uint64_t b = (v ^ a) * mul;
	b ^= (b >> 47);
	b *= mul;
	return b;
}

static uint64_t _cityHashHashLen0to16(const char *s, size_t len) {
	if (len >= 8) {
		uint64_t mul = k2 + len * 2;
		uint64_t a = _cityHashFetch64(s) + k2;
		uint64_t b = _cityHashFetch64(s + len - 8);
		uint64_t c = peff::rRot(b, 37) * mul + a;
		uint64_t d = (peff::rRot(a, 25) + b) * mul;
		return _cityHashHashLen16(c, d, mul);
	}
	if (len >= 4) {
		uint64_t mul = k2 + len * 2;
		uint64_t a = _cityHashFetch32(s);
		return _cityHashHashLen16(len + (a << 3), _cityHashFetch32(s + len - 4), mul);
	}
	if (len > 0) {
		uint8_t a = static_cast<uint8_t>(s[0]);
		uint8_t b = static_cast<uint8_t>(s[len >> 1]);
		uint8_t c = static_cast<uint8_t>(s[len - 1]);
		uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
		uint32_t z = static_cast<uint32_t>(len) + (static_cast<uint32_t>(c) << 2);
		return _cityHashShiftMix(y * k2 ^ z * k0) * k2;
	}
	return k2;
}

static uint64_t _cityHashHashLen17to32(const char *s, size_t len) {
	uint64_t mul = k2 + len * 2;
	uint64_t a = _cityHashFetch64(s) * k1;
	uint64_t b = _cityHashFetch64(s + 8);
	uint64_t c = _cityHashFetch64(s + len - 8) * mul;
	uint64_t d = _cityHashFetch64(s + len - 16) * k2;
	return _cityHashHashLen16(peff::rRot(a + b, 43) + peff::rRot(c, 30) + d,
		a + peff::rRot(b + k2, 18) + c, mul);
}

static std::pair<uint64_t, uint64_t> _cityHashWeakHashLen32WithSeeds(
	uint64_t w, uint64_t x, uint64_t y, uint64_t z, uint64_t a, uint64_t b) {
	a += w;
	b = peff::rRot(b + a + z, 21);
	uint64_t c = a;
	a += x;
	a += y;
	b += peff::rRot(a, 44);
	return std::make_pair(a + z, b + c);
}

static std::pair<uint64_t, uint64_t> _cityHashWeakHashLen32WithSeeds(
	const char *s, uint64_t a, uint64_t b) {
	return _cityHashWeakHashLen32WithSeeds(_cityHashFetch64(s),
		_cityHashFetch64(s + 8),
		_cityHashFetch64(s + 16),
		_cityHashFetch64(s + 24),
		a,
		b);
}

static uint64_t _cityHashHashLen33to64(const char *s, size_t len) {
	uint64_t mul = k2 + len * 2;
	uint64_t a = _cityHashFetch64(s) * k2;
	uint64_t b = _cityHashFetch64(s + 8);
	uint64_t c = _cityHashFetch64(s + len - 24);
	uint64_t d = _cityHashFetch64(s + len - 32);
	uint64_t e = _cityHashFetch64(s + 16) * k2;
	uint64_t f = _cityHashFetch64(s + 24) * 9;
	uint64_t g = _cityHashFetch64(s + len - 8);
	uint64_t h = _cityHashFetch64(s + len - 16) * mul;
	uint64_t u = peff::rRot(a + g, 43) + (peff::rRot(b, 30) + c) * 9;
	uint64_t v = ((a + g) ^ d) + f + 1;
	uint64_t w = peff::swapByteOrder((u + v) * mul) + h;
	uint64_t x = peff::rRot(e + f, 42) + c;
	uint64_t y = (peff::swapByteOrder((v + w) * mul) + g) * mul;
	uint64_t z = e + f + c;
	a = peff::swapByteOrder((x + z) * mul + y) + b;
	b = _cityHashShiftMix((z + a) * mul + d + h) * mul;
	return b + x;
}

PEFF_UTILS_API uint64_t peff::cityHash64(const char *s, size_t len) {
	if (len <= 32) {
		if (len <= 16) {
			return _cityHashHashLen0to16(s, len);
		} else {
			return _cityHashHashLen17to32(s, len);
		}
	} else if (len <= 64) {
		return _cityHashHashLen33to64(s, len);
	}

	// For strings over 64 bytes we hash the end first, and then as we
	// loop we keep 56 bytes of state: v, w, x, y, and z.
	uint64_t x = _cityHashFetch64(s + len - 40);
	uint64_t y = _cityHashFetch64(s + len - 16) + _cityHashFetch64(s + len - 56);
	uint64_t z = _cityHashHashLen16(_cityHashFetch64(s + len - 48) + len, _cityHashFetch64(s + len - 24));
	std::pair<uint64_t, uint64_t> v = _cityHashWeakHashLen32WithSeeds(s + len - 64, len, z);
	std::pair<uint64_t, uint64_t> w = _cityHashWeakHashLen32WithSeeds(s + len - 32, y + k1, x);
	x = x * k1 + _cityHashFetch64(s);

	// Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
	len = (len - 1) & ~static_cast<size_t>(63);
	do {
		x = peff::rRot(x + y + v.first + _cityHashFetch64(s + 8), 37) * k1;
		y = peff::rRot(y + v.second + _cityHashFetch64(s + 48), 42) * k1;
		x ^= w.second;
		y += v.first + _cityHashFetch64(s + 40);
		z = peff::rRot(z + w.first, 33) * k1;
		v = _cityHashWeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
		w = _cityHashWeakHashLen32WithSeeds(s + 32, z + w.second, y + _cityHashFetch64(s + 16));
		std::swap(z, x);
		s += 64;
		len -= 64;
	} while (len != 0);
	return _cityHashHashLen16(_cityHashHashLen16(v.first, w.first) + _cityHashShiftMix(y) * k1 + z,
		_cityHashHashLen16(v.second, w.second) + x);
}
