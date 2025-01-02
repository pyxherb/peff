#ifndef _PEFF_CONTAINERS_HASHSET_H_
#define _PEFF_CONTAINERS_HASHSET_H_

#include "list.h"
#include "dynarray.h"
#include "misc.h"
#include <peff/utils/hash.h>
#include <peff/utils/comparator.h>
#include <peff/utils/scope_guard.h>
#include <memory>

namespace peff {
	template <
		typename T,
		typename EqCmp = EqComparator<T>,
		typename Hasher = Hasher<T>>
	class HashSet {
	public:
		using HashCode = decltype(std::declval<Hasher>()(std::declval<T>()));

		struct Element {
			T data;
			HashCode hashCode;

			Element() = default;
			PEFF_FORCEINLINE Element(const T &data, HashCode hashCode) : data(data), hashCode(hashCode) {}
			PEFF_FORCEINLINE Element(T &&data, HashCode hashCode) : data(data), hashCode(hashCode) {}
		};
		using Bucket = List<Element>;

	private:
		using ThisType = HashSet<T, EqCmp, Hasher>;

		using BucketsType = DynArray<Bucket>;
		BucketsType _buckets;

		size_t _size = 0;
		EqCmp _equalityComparator;
		Hasher _hasher;

		PEFF_FORCEINLINE int _checkCapacity() {
			size_t capacity = _buckets.size() << 1;

			if (capacity < _size)
				return 1;
			if (auto size = (capacity >> 1); size > _size) {
				if (size <= 1)
					return 0;
				return -1;
			}
			return 0;
		}

		[[nodiscard]] PEFF_FORCEINLINE static bool _resizeBuckets(size_t newSize, BucketsType &oldBuckets, BucketsType &newBuckets) {
			{
				Bucket fillerBucket(oldBuckets.allocator());
				if (!newBuckets.resizeWith(newSize, fillerBucket)) {
					return false;
				}
			}

			const size_t nOldBuckets = oldBuckets.size();
			ScopeGuard restoreGuard([newSize, &oldBuckets, &newBuckets, nOldBuckets]() {
				for (size_t i = 0; i < newSize; ++i) {
					Bucket &bucket = newBuckets.at(i);

					for (Bucket::NodeHandle j = bucket.firstNode(); j; j = j->next) {
						size_t index = ((size_t)j->data.hashCode) % nOldBuckets;

						bucket.detach(j);
						oldBuckets.at(index).pushFront(j);
					}
				}
				newBuckets.clear();
			});

			for (size_t i = 0; i < nOldBuckets; ++i) {
				Bucket &bucket = oldBuckets.at(i);

				for (Bucket::NodeHandle j = bucket.firstNode(); j; j = j->next) {
					size_t index = ((size_t)j->data.hashCode) % newSize;

					bucket.detach(j);
					newBuckets.at(index).pushFront(j);
				}
			}

			restoreGuard.release();
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle _getBucketSlot(const Bucket &bucket, const T &data) const {
			for (auto i = bucket.firstNode(); i; i = i->next) {
				if (_equalityComparator(i->data.data, data)) {
					return i;
				}
			}

			return Bucket::nullNodeHandle();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _checkAndResizeBuckets() {
			size_t size = _buckets.size();

			switch (_checkCapacity()) {
				case 1: {
					BucketsType newBuckets(_buckets.allocator());
					if (!_resizeBuckets(size ? size << 1 : 1, _buckets, newBuckets)) {
						return false;
					}
					_buckets = std::move(newBuckets);
					break;
				}
				case 0:
					break;
				case -1: {
					BucketsType newBuckets(_buckets.allocator());
					if (!_resizeBuckets(size >> 1, _buckets, newBuckets)) {
						return false;
					}
					_buckets = std::move(newBuckets);
					break;
				}
			}

			return true;
		}

		PEFF_FORCEINLINE bool _isBucketsInitialized() {
			return _buckets.size();
		}

		/// @brief Insert a new element.
		/// @param buckets Buckets to be operated.
		/// @param data Element to insert.
		/// @return true for succeeded, false if failed.
		[[nodiscard]] PEFF_FORCEINLINE bool _insert(T &&data) {
			if (!_buckets.size()) {
				if (!_buckets.resize(1)) {
					return false;
				}
			}

			HashCode hashCode = _hasher(data);
			size_t index = ((size_t)hashCode) % _buckets.size();
			Bucket &bucket = _buckets.at(index);

			T tmpData = std::move(data);

			if (_getBucketSlot(bucket, tmpData))
				return true;

			if (!bucket.pushFront(Element(std::move(tmpData), hashCode)))
				return false;

			if (!_checkAndResizeBuckets()) {
				bucket.popFront();
				return false;
			}

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _remove(const T &data) {
			if (!_buckets.size()) {
				return false;
			}

			HashCode hashCode = _hasher(data);
			size_t index = ((size_t)hashCode) % _buckets.size();
			Bucket &bucket = _buckets.at(index);

			Bucket::NodeHandle node = _getBucketSlot(bucket, data);
			Bucket::NodeHandle nextNode = Bucket::next(node, 1);

			bucket.detach(node);

			if (!_checkAndResizeBuckets()) {
				if (nextNode) {
					bucket.insertFront(nextNode, node);
				} else {
					bucket.pushFront(node);
				}
				return false;
			}

			bucket.deleteNode(node);

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle _get(const T &data, size_t &index) const {
			if (!_buckets.size()) {
				return Bucket::nullNodeHandle();
			}

			HashCode hashCode = _hasher(data);
			size_t index = ((size_t)hashCode) % _buckets.size();
			const Bucket &bucket = _buckets.at(index);

			return _getBucketSlot(bucket, data);
		}

	public:
		PEFF_FORCEINLINE HashSet(Alloc *allocator = getDefaultAlloc()) : _buckets(allocator) {
		}

		PEFF_FORCEINLINE HashSet(ThisType &&other) {
			_buckets = std::move(other._buckets);
			_size = other._size;
			_equalityComparator = std::move(other._equalityComparator);
			_hasher = std::move(other._hasher);
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) {
			_buckets = std::move(other._buckets);
			_size = other._size;
			_equalityComparator = std::move(other._equalityComparator);
			_hasher = std::move(other._hasher);

			other._size = 0;

			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&data) {
			return _insert(std::move(data));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool remove(const T &data) {
			return _remove(data);
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle get(const T &data) {
			size_t index;
			return _get(data, index);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool contains(const T &data) const {
			size_t index;
			return _get(data, index);
		}

		PEFF_FORCEINLINE void clear() {
			_buckets.clear();
		}

		PEFF_FORCEINLINE Alloc *allocator() {
			return _buckets.allocator();
		}

		struct Iterator {
			size_t idxCurBucket;
			typename Bucket::NodeHandle bucketNodeHandle;
			ThisType *hashSet;
			IteratorDirection direction;

			PEFF_FORCEINLINE Iterator(
				ThisType *hashSet,
				size_t idxCurBucket,
				typename Bucket::NodeHandle bucketNodeHandle,
				IteratorDirection direction)
				: idxCurBucket(idxCurBucket),
				  bucketNodeHandle(bucketNodeHandle),
				  hashSet(hashSet),
				  direction(direction) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				idxCurBucket = it.idxCurBucket;
				bucketNodeHandle = it.bucketNodeHandle;
				hashSet = it.hashSet;
				direction = it.direction;

				it.idxCurBucket = SIZE_MAX;
				it.bucketNodeHandle = Bucket::nullNodeHandle();
				it.hashSet = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				idxCurBucket = rhs.idxCurBucket;
				bucketNodeHandle = rhs.bucketNodeHandle;
				hashSet = rhs.hashSet;
				return *this;
			}
			PEFF_FORCEINLINE Iterator &operator=(Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				new (this) Iterator(rhs);
				return *this;
			}

			PEFF_FORCEINLINE bool copy(Iterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				if (idxCurBucket == SIZE_MAX)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					Bucket::NodeHandle nextNode = Bucket::next(bucketNodeHandle, 1);
					if (!nextNode) {
						while ((!nextNode) && (idxCurBucket != SIZE_MAX)) {
							if (++idxCurBucket >= hashSet->_buckets.size()) {
								idxCurBucket = SIZE_MAX;
								nextNode = nullptr;
							} else {
								nextNode = hashSet->_buckets.at(idxCurBucket).firstNode();
							}
						}
					}
					bucketNodeHandle = nextNode;
				} else {
					Bucket::NodeHandle nextNode = Bucket::prev(bucketNodeHandle, 1);
					if (!nextNode) {
						while ((!nextNode) && (idxCurBucket != SIZE_MAX)) {
							if (!idxCurBucket) {
								idxCurBucket = SIZE_MAX;
								nextNode = nullptr;
							} else {
								--idxCurBucket;
								nextNode = hashSet->_buckets.at(idxCurBucket).lastNode();
							}
						}
					}
					bucketNodeHandle = nextNode;
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++(*this);
				return it;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (idxCurBucket == SIZE_MAX) {
						idxCurBucket = hashSet->_buckets.size();
						bucketNodeHandle = hashSet->_buckets.at(idxCurBucket).lastNode();
					} else {
						Bucket::NodeHandle nextNode = Bucket::prev(bucketNodeHandle, 1);
						if (!nextNode) {
							while (!nextNode) {
								if (!idxCurBucket) {
									throw std::logic_error("Decreasing the beginning iterator");
								} else {
									--idxCurBucket;
									nextNode = hashSet->_buckets.at(idxCurBucket).lastNode();
								}
							}
						}
						bucketNodeHandle = nextNode;
					}
				} else {
					if (idxCurBucket == SIZE_MAX) {
						idxCurBucket = 0;
						bucketNodeHandle = hashSet->_buckets.at(0).firstNode();
					} else {
						Bucket::NodeHandle nextNode = Bucket::next(bucketNodeHandle, 1);
						if (!nextNode) {
							while (!nextNode) {
								if (++idxCurBucket >= hashSet->_buckets.size()) {
									throw std::logic_error("Decreasing the beginning iterator");
								} else {
									nextNode = hashSet->_buckets.at(idxCurBucket).firstNode();
								}
							}
						}
						bucketNodeHandle = nextNode;
					}
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				if (hashSet != it.hashSet)
					throw std::logic_error("Cannot compare iterators from different containers");
				return bucketNodeHandle == it.bucketNodeHandle;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &&rhs) const {
				const Iterator it = rhs;
				return *this == it;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &it) const {
				if (hashSet != it.hashSet)
					throw std::logic_error("Cannot compare iterators from different containers");
				return bucketNodeHandle != it.bucketNodeHandle;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE T &operator*() {
				if (!bucketNodeHandle)
					throw std::logic_error("Deferencing the end iterator");
				return bucketNodeHandle->data.data;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!bucketNodeHandle)
					throw std::logic_error("Deferencing the end iterator");
				return bucketNodeHandle->data.data;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!bucketNodeHandle)
					throw std::logic_error("Deferencing the end iterator");
				return &bucketNodeHandle->data.data;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!bucketNodeHandle)
					throw std::logic_error("Deferencing the end iterator");
				return &bucketNodeHandle->data.data;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			for (size_t i = 0; i < _buckets.size(); ++i) {
				auto &curBucket = _buckets.at(i);
				Bucket::NodeHandle node = curBucket.firstNode();
				if (node) {
					return Iterator(this, i, node, IteratorDirection::Forward);
				}
			}
			return end();
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(this, SIZE_MAX, nullptr, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator beginReversed() {
			for (size_t i = _buckets.size(); i; --i) {
				auto &curBucket = _buckets.at(i);
				Bucket::NodeHandle node = curBucket.lastNode();
				if (node) {
					return Iterator(this, i, node, IteratorDirection::Reversed);
				}
			}

			auto &curBucket = _buckets.at(0);
			Bucket::NodeHandle node = curBucket.lastNode();
			if (node) {
				return Iterator(this, 0, node, IteratorDirection::Reversed);
			}
			return endReversed();
		}

		PEFF_FORCEINLINE Iterator endReversed() {
			return Iterator(this, SIZE_MAX, nullptr, IteratorDirection::Reversed);
		}

		struct ConstIterator {
			Iterator _iterator;
			PEFF_FORCEINLINE ConstIterator(Iterator &&iteratorIn) : _iterator(iteratorIn) {
			}
			ConstIterator(const ConstIterator &rhs) = default;
			ConstIterator(ConstIterator &&rhs) = default;
			ConstIterator &operator=(const ConstIterator &rhs) = default;
			ConstIterator &operator=(ConstIterator &&rhs) = default;

			PEFF_FORCEINLINE bool operator==(const ConstIterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(ConstIterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE T &operator*() {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE T *operator->() {
				return &*_iterator;
			}

			PEFF_FORCEINLINE const T *operator->() const {
				return &*_iterator;
			}
		};

		PEFF_FORCEINLINE ConstIterator beginConst() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator endConst() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator beginConstReversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->beginReversed());
		}
		PEFF_FORCEINLINE ConstIterator endConstReversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->endReversed());
		}

		PEFF_FORCEINLINE Iterator find(const T &value) {
			size_t index;
			Bucket::NodeHandle node = _get(value, index);
			if (!node)
				return end();
			return Iterator(this, index, node, IteratorDirection::Forward);
		}

		PEFF_FORCEINLINE ConstIterator find(const T &value) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(value));
		}

		T& at(const T& value) {
			size_t index;
			Bucket::NodeHandle node = _get(value, index);
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		const T &at(const T &value) const {
			return const_cast<ThisType *>(this)->at(value);
		}
	};
}

#endif
