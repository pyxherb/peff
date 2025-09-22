#ifndef _PEFF_CONTAINERS_HASHSET_H_
#define _PEFF_CONTAINERS_HASHSET_H_

#include "list.h"
#include "dynarray.h"
#include "option.h"
#include "fallible_cmp.h"
#include "fallible_hash.h"
#include "misc.h"
#include <peff/utils/hash.h>
#include <peff/base/scope_guard.h>
#include <memory>

#if __cplusplus >= 202002L
	#include <concepts>
#endif

namespace peff {
	template <
		typename T,
		typename EqCmp,
		typename Hasher,
		bool Fallible>
	PEFF_REQUIRES_CONCEPT(std::invocable<EqCmp, const T &, const T &>)
	class HashSetImpl {
	private:
		template <typename T2, typename V = void>
		struct HashCodePassUtil {
			using type = T2;
		};

		template <typename T2>
		struct HashCodePassUtil<T2, std::void_t<decltype(std::declval<T2>().value())>> {
			using type = typename T2::value_type;
		};

	public:
		using HasherResult = decltype(std::declval<Hasher>()(std::declval<T>()));
		using HashCode = std::conditional_t<Fallible, typename HashCodePassUtil<HasherResult>::type, HasherResult>;

		struct Element {
			T data;
			HashCode hashCode;

			PEFF_FORCEINLINE Element(T &&data, HashCode hashCode) : data(std::move(data)), hashCode(hashCode) {}

			Element(Element &&rhs) = default;
			Element &operator=(Element &&rhs) = default;

			PEFF_FORCEINLINE bool copy(Element &dest) const {
				peff::Uninitialized<T> copiedData;

				if (!copiedData.copyFrom(data)) {
					return false;
				}

				peff::constructAt(&dest, copiedData.release(), hashCode);

				return true;
			}
		};
		using Bucket = List<Element>;

	private:
		template <bool Fallible2>
		struct InternalRemoveResultTypeUtil {
			using type = bool;
		};

		template <>
		struct InternalRemoveResultTypeUtil<true> {
			using type = Option<bool>;
		};

		template <bool Fallible2>
		struct RemoveResultTypeUtil {
			using type = void;
		};

		template <>
		struct RemoveResultTypeUtil<true> {
			using type = bool;
		};

		template <bool Fallible2>
		struct RemoveAndResizeResultTypeUtil {
			using type = bool;
		};

		template <>
		struct RemoveAndResizeResultTypeUtil<true> {
			using type = Option<bool>;
		};

		template <bool Fallible2>
		struct BucketNodeHandleQueryResultTypeUtil {
			using type = typename Bucket::NodeHandle;
		};

		template <>
		struct BucketNodeHandleQueryResultTypeUtil<true> {
			using type = Option<typename Bucket::NodeHandle>;
		};

		template <bool Fallible2>
		struct ElementQueryResultTypeUtil {
			using type = T &;
		};

		template <>
		struct ElementQueryResultTypeUtil<true> {
			using type = Option<T &>;
		};

		template <bool Fallible2>
		struct ConstElementQueryResultTypeUtil {
			using type = T &;
		};

		template <>
		struct ConstElementQueryResultTypeUtil<true> {
			using type = Option<T &>;
		};

		template <bool Fallible2>
		struct ContainsResultTypeUtil {
			using type = bool;
		};

		template <>
		struct ContainsResultTypeUtil<true> {
			using type = Option<bool>;
		};

	public:
		using InternalRemoveResultType = typename InternalRemoveResultTypeUtil<Fallible>::type;
		using RemoveResultType = typename RemoveResultTypeUtil<Fallible>::type;
		using RemoveAndResizeResultType = typename RemoveAndResizeResultTypeUtil<Fallible>::type;
		using ElementQueryResultType = typename ElementQueryResultTypeUtil<Fallible>::type;
		using BucketNodeHandleQueryResultType = typename BucketNodeHandleQueryResultTypeUtil<Fallible>::type;
		using ConstElementQueryResultType = typename ConstElementQueryResultTypeUtil<Fallible>::type;
		using ContainsResultType = typename ContainsResultTypeUtil<Fallible>::type;

	private:
		using ThisType = HashSetImpl<T, EqCmp, Hasher, Fallible>;

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
				if (!newBuckets.resizeUninitialized(newSize)) {
					return false;
				}
				for (size_t i = 0; i < newBuckets.size(); ++i) {
					peff::constructAt<Bucket>(&newBuckets.at(i), newBuckets.allocator());
				}
			}

			const size_t nOldBuckets = oldBuckets.size();
			ScopeGuard restoreGuard([newSize, &oldBuckets, &newBuckets, nOldBuckets]() noexcept {
				for (size_t i = 0; i < newSize; ++i) {
					Bucket &bucket = newBuckets.at(i);

					for (typename Bucket::NodeHandle j = bucket.firstNode(); j; j = j->next) {
						size_t index = ((size_t)j->data.hashCode) % nOldBuckets;

						bucket.detach(j);
						oldBuckets.at(index).pushFront(j);
					}
				}
				newBuckets.clear();
			});

			for (size_t i = 0; i < nOldBuckets; ++i) {
				Bucket &bucket = oldBuckets.at(i);

				for (typename Bucket::NodeHandle j = bucket.firstNode(); j;) {
					typename Bucket::NodeHandle next = j->next;
					size_t index = ((size_t)j->data.hashCode) % newSize;

					bucket.detach(j);
					newBuckets.at(index).pushFront(j);
					j = next;
				}
			}

			restoreGuard.release();

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE BucketNodeHandleQueryResultType _getBucketSlot(const Bucket &bucket, const T &data) const {
			for (auto i = bucket.firstNode(); i; i = i->next) {
				if constexpr (Fallible) {
					if (auto result = _equalityComparator(i->data.data, data); result.hasValue()) {
						return i;
					} else {
						return NULL_OPTION;
					}
				} else {
					if (_equalityComparator(i->data.data, data)) {
						return i;
					}
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
		[[nodiscard]] PEFF_FORCEINLINE bool _insert(T &&data, bool forceResizeBuckets) {
			if (!_buckets.size()) {
				if (!_buckets.resizeUninitialized(1)) {
					return false;
				}

				peff::constructAt<Bucket>(&_buckets.at(0), _buckets.allocator());
			}

			T tmpData = std::move(data);

			HashCode hashCode;
			if constexpr (Fallible) {
				if (auto result = _hasher(tmpData); result.hasValue()) {
					hashCode = result.value();
				} else
					return false;
			} else {
				hashCode = _hasher(tmpData);
			}
			size_t index = ((size_t)hashCode) % _buckets.size();
			Bucket &bucket = _buckets.at(index);

			if (!bucket.pushFront(Element(std::move(tmpData), hashCode)))
				return false;

			if (!_checkAndResizeBuckets()) {
				if (forceResizeBuckets) {
					bucket.popFront();
					return false;
				}
			}

			++_size;
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE InternalRemoveResultType _remove(const T &data, bool forceResizeBuckets) {
			if (!_buckets.size()) {
				return false;
			}

			HashCode hashCode;
			if constexpr (Fallible) {
				if (auto result = _hasher(data); result.hasValue()) {
					hashCode = result.value();
				} else
					return NULL_OPTION;
			} else {
				hashCode = _hasher(data);
			}
			size_t index = ((size_t)hashCode) % _buckets.size();
			Bucket &bucket = _buckets.at(index);
			peff::RcObjectPtr<Alloc> alloc = bucket.allocator();

			typename Bucket::NodeHandle node;

			if constexpr (Fallible) {
				BucketNodeHandleQueryResultType maybeNode = _getBucketSlot(bucket, data);
				if (!maybeNode.hasValue()) {
					return NULL_OPTION;
				}

				node = maybeNode.value();
			} else {
				node = _getBucketSlot(bucket, data);
			}

			if (node) {
				typename Bucket::NodeHandle nextNode = Bucket::next(node, 1);

				bucket.detach(node);

				Bucket deleterBucket(alloc.get());

				if (!_checkAndResizeBuckets()) {
					if (forceResizeBuckets) {
						if (nextNode) {
							typename Bucket::NodeHandle result = bucket.insertFront(nextNode, node);
							assert(result);
						} else {
							bucket.pushFront(node);
						}
						return false;
					}
				}

				deleterBucket.deleteNode(node);

				--_size;
			}
			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE BucketNodeHandleQueryResultType _get(const T &data, size_t &index) const {
			if (!_buckets.size()) {
				return Bucket::nullNodeHandle();
			}

			HashCode hashCode;
			if constexpr (Fallible) {
				if (auto result = _hasher(data); result.hasValue()) {
					hashCode = result.value();
				} else
					return NULL_OPTION;
			} else {
				hashCode = _hasher(data);
			}
			size_t i = ((size_t)hashCode) % _buckets.size();
			const Bucket &bucket = _buckets.at(i);

			return _getBucketSlot(bucket, data);
		}

	public:
		PEFF_FORCEINLINE HashSetImpl(Alloc *allocator) : _buckets(allocator) {
		}

		PEFF_FORCEINLINE HashSetImpl(ThisType &&other)
			: _buckets(std::move(other._buckets)),
			  _size(other._size),
			  _equalityComparator(std::move(other._equalityComparator)),
			  _hasher(std::move(other._hasher)) {
			other._size = 0;
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) noexcept {
			clear();

			_buckets = std::move(other._buckets);
			_size = other._size;
			_equalityComparator = std::move(other._equalityComparator);
			_hasher = std::move(other._hasher);

			other._size = 0;

			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&data) {
			return _insert(std::move(data), false);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertAndResizeBuckets(T &&data) {
			return _insert(std::move(data), true);
		}

		[[nodiscard]] PEFF_FORCEINLINE RemoveResultType remove(const T &data) {
			if constexpr (Fallible) {
				return _remove(data, false).hasValue();
			} else {
				bool unused = _remove(data, false);
			}
		}

		[[nodiscard]] PEFF_FORCEINLINE RemoveAndResizeResultType removeAndResizeBuckets(const T &data) {
			if constexpr (Fallible) {
				auto result = _remove(data, true);

				if (!result.hasValue())
					return NULL_OPTION;
			} else {
				return _remove(data, true);
			}
		}

		[[nodiscard]] PEFF_FORCEINLINE BucketNodeHandleQueryResultType get(const T &data) {
			size_t index;
			return _get(data, index);
		}

		[[nodiscard]] PEFF_FORCEINLINE ContainsResultType contains(const T &data) const {
			size_t index;
			if constexpr (Fallible) {
				auto maybeHandle = _get(data, index);

				if (!maybeHandle.hasValue())
					return NULL_OPTION;

				return maybeHandle.value();
			} else {
				return _get(data, index);
			}
		}

		PEFF_FORCEINLINE void clear() {
			_buckets.clear();
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _buckets.allocator();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) noexcept {
			for (auto &i : _buckets) {
				i.replaceAllocator(rhs);
			}
			_buckets.replaceAllocator(rhs);
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
				constructAt<Iterator>(this, std::move(rhs));
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
					typename Bucket::NodeHandle nextNode = Bucket::next(bucketNodeHandle, 1);
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
					typename Bucket::NodeHandle nextNode = Bucket::prev(bucketNodeHandle, 1);
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
						typename Bucket::NodeHandle nextNode = Bucket::prev(bucketNodeHandle, 1);
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
						typename Bucket::NodeHandle nextNode = Bucket::next(bucketNodeHandle, 1);
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
				typename Bucket::NodeHandle node = curBucket.firstNode();
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
				typename Bucket::NodeHandle node = curBucket.lastNode();
				if (node) {
					return Iterator(this, i, node, IteratorDirection::Reversed);
				}
			}

			auto &curBucket = _buckets.at(0);
			typename Bucket::NodeHandle node = curBucket.lastNode();
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

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
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
			if constexpr (Fallible) {
				BucketNodeHandleQueryResultType node = _get(value, index);
				if (!node.hasValue())
					return end();
				if (typename Bucket::NodeHandle handle = node.value(); handle)
					return Iterator(this, index, handle, IteratorDirection::Forward);
				return end();
			} else {
				typename Bucket::NodeHandle node = _get(value, index);
				if (!node)
					return end();
				return Iterator(this, index, node, IteratorDirection::Forward);
			}
		}

		PEFF_FORCEINLINE ConstIterator find(const T &value) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(value));
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const T &value) {
			size_t index;
			typename Bucket::NodeHandle node;
			if constexpr (Fallible) {
				auto maybeNode = _get(value, index);

				if (!maybeNode.hasValue())
					return NULL_OPTION;

				node = maybeNode.value();
			} else {
				node = _get(value, index);
			}
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const T &value) const {
			size_t index;
			typename Bucket::NodeHandle node;
			if constexpr (Fallible) {
				auto maybeNode = _get(value, index);

				if (!maybeNode.hasValue())
					return NULL_OPTION;

				node = maybeNode.value();
			} else {
				node = _get(value, index);
			}
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _size;
		}

		PEFF_FORCEINLINE bool copy(ThisType &dest) const {
			constructAt<ThisType>(&dest, allocator());

			ScopeGuard clearDestGuard([&dest]() noexcept {
				dest.clear();
			});

			for (ConstIterator i = beginConst(); i != endConst(); ++i) {
				Uninitialized<T> copiedData;

				if (!copiedData.copyFrom(*i)) {
					return false;
				}

				if (!dest.insert(copiedData.release())) {
					return false;
				}
			}

			clearDestGuard.release();

			return true;
		}
	};

	template <typename T, typename EqCmp = std::equal_to<T>, typename Hasher = peff::Hasher<T>>
	using HashSet = HashSetImpl<T, EqCmp, Hasher, false>;
	template <typename T, typename EqCmp = peff::FallibleEq<T>, typename Hasher = peff::FallibleHasher<T>>
	using FallibleHashSet = HashSetImpl<T, EqCmp, Hasher, true>;
}

#endif
