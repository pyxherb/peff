#ifndef _PHUL_CONTAINERS_HASHSET_H_
#define _PHUL_CONTAINERS_HASHSET_H_

#include "list.h"
#include "dynarray.h"
#include "misc.h"
#include <phul/utils/hash.h>
#include <phul/utils/comparator.h>

constexpr size_t HASHSET_MIN_BUCKET_SIZE = 4;

namespace phul {
	template <
		typename T,
		typename EqCmp = EqComparator<T>,
		typename Hasher = Hasher<T>,
		typename Allocator = StdAlloc>
	class HashSet {
	public:
		struct Element {
			T data;
			HashCode hashCode;

			PHUL_FORCEINLINE Element(const T &data, HashCode hashCode) : data(data), hashCode(hashCode) {}
			PHUL_FORCEINLINE Element(T &&data, HashCode hashCode) : data(data), hashCode(hashCode) {}
		};
		using Bucket = List<Element>;

	private:
		using BucketsType = DynArray<Bucket, Allocator>;
		using ThisType = HashSet<T, EqCmp, Hasher, Allocator>;
		BucketsType _buckets;
		size_t _size = 0;
		EqCmp _equalityComparator;
		Hasher _hasher;
		Allocator _allocator;

		PHUL_FORCEINLINE int _checkCapacity() {
			size_t capacity = _buckets.getSize() << 1;

			if (capacity < _size)
				return 1;
			if (auto size = (capacity >> 1); size > _size) {
				if (size < HASHSET_MIN_BUCKET_SIZE)
					return 0;
				return -1;
			}
			return 0;
		}

		PHUL_FORCEINLINE void _resizeBuckets(size_t newSize) {
			BucketsType newBuckets;
			newBuckets.resize(newSize);

			const size_t nOldBuckets = _buckets.getSize();

			try {
				for (size_t i = 0; i < nOldBuckets; ++i) {
					Bucket &bucket = _buckets.at(i);

					for (Bucket::Node *j = bucket.firstNode(); j; j = j->next) {
						size_t index = ((size_t)j->data.hashCode) % newSize;

						bucket.detach(j);
						newBuckets.at(index).pushFront(j);
					}
				}
			} catch (...) {
				for (size_t i = 0; i < newSize; ++i) {
					Bucket &bucket = newBuckets.at(i);

					for (Bucket::Node *j = bucket.firstNode(); j; j = j->next) {
						size_t index = ((size_t)j->data.hashCode) % nOldBuckets;

						bucket.detach(j);
						newBuckets.at(index).pushFront(j);
					}
				}
				std::rethrow_exception(std::current_exception());
			}
		}

		/// @brief Insert a new element.
		/// @param buckets Buckets to be operated.
		/// @param data Element to insert.
		/// @return true for succeeded, false if failed.
		PHUL_FORCEINLINE bool _insert(BucketsType &buckets, T &&data) {
			HashCode hashCode = _hasher(data);
			size_t index = ((size_t)hashCode) % buckets.getSize();
			Bucket &bucket = buckets.at(index);

			for (auto i = bucket.firstNode(); i; i = i->next) {
				if (_equalityComparator(i->data.data, data))
					break;
			}

			if (!bucket.pushFront(Element(data, hashCode)))
				return false;

			switch (_checkCapacity()) {
				case 1:
					_resizeBuckets(buckets.getSize() << 1);
					break;
				case 0:
					break;
				case -1:
					_resizeBuckets(buckets.getSize() >> 1);
					break;
			}

			return true;
		}

	public:
		PHUL_FORCEINLINE HashSet() {
		}

		PHUL_FORCEINLINE HashSet(const ThisType &other) {
			_buckets = other._buckets;
			_size = other._size;
			_equalityComparator = other._equalityComparator;
			_hasher = other._hasher;
			_allocator = other._allocator;
		}
		PHUL_FORCEINLINE HashSet(ThisType &&other) {
			_buckets = std::move(other._buckets);
			_size = other._size;
			_equalityComparator = std::move(other._equalityComparator);
			_hasher = std::move(other._hasher);
			_allocator = std::move(other._allocator);
		}

		PHUL_FORCEINLINE ThisType &operator=(const ThisType &other) {
			_buckets = other._buckets;
			_size = other._size;
			_equalityComparator = other._equalityComparator;
			_hasher = other._hasher;
			_allocator = other._allocator;

			return *this;
		}
		PHUL_FORCEINLINE ThisType &operator=(ThisType &&other) {
		}
	};
}

#endif
