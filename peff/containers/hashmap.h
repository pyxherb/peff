#ifndef _PEFF_CONTAINERS_HASHMAP_H_
#define _PEFF_CONTAINERS_HASHMAP_H_

#include "hashset.h"

namespace peff {
	template <typename K, typename V, typename Eq = EqComparator<K>, typename Hasher = Hasher<K>>
	class HashMap final {
	private:
		struct Pair {
			K key;
			V value;
			bool isForQuery;

			Pair(Pair &&rhs) = default;
		};

		struct QueryPair : Pair {
			const K *queryKey;
		};

		struct PairComparator {
			Eq eqComparator;

			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.isForQuery ? *((const QueryPair &)lhs).queryKey : lhs.key,
						&r = rhs.isForQuery ? *((const QueryPair &)rhs).queryKey : rhs.key;
				return eqComparator(l, r);
			}
		};

		struct PairHasher {
			Hasher hasher;

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const Pair &pair) const {
				const K &k = pair.isForQuery ? *((const QueryPair &)pair).queryKey : pair.key;
				return hasher(k);
			}
		};

		PairComparator comparator;

		using SetType = HashSet<Pair, PairComparator, PairHasher>;

		SetType _set;

		using ThisType = HashMap<K, V, Eq, Hasher>;

		PEFF_FORCEINLINE static void _constructKeyOnlyPairByCopy(const K &key, char *dest) {
			((QueryPair *)dest)->isForQuery = true;
			((QueryPair *)dest)->queryKey = &key;
		}

	public:
		PEFF_FORCEINLINE HashMap(Alloc *allocator) : _set(allocator) {}
		PEFF_FORCEINLINE HashMap(ThisType &&rhs) : comparator(std::move(rhs.comparator)), _set(std::move(rhs._set)) {
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			clear();

			comparator = std::move(rhs.comparator);
			_set = std::move(rhs._set);

			return *this;
		}

		PEFF_FORCEINLINE bool insert(K &&key, V &&value) {
			Pair pair = Pair{ std::move(key), std::move(value), false };
			return _set.insert(std::move(pair));
		}

		PEFF_FORCEINLINE bool insertAndResizeBuckets(K &&key, V &&value) {
			Pair pair = Pair{ std::move(key), std::move(value), false };
			return _set.insertAndResizeBuckets(std::move(pair));
		}

		PEFF_FORCEINLINE void remove(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			_set.remove(*(QueryPair *)pair);
		}

		PEFF_FORCEINLINE bool removeAndResizeBuckets(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.removeAndResizeBuckets(*(QueryPair *)pair);
		}

		PEFF_FORCEINLINE bool contains(const K &key) const {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.contains(*(QueryPair *)pair);
		}

		PEFF_FORCEINLINE V &at(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.at(*(QueryPair *)pair).value;
		}

		PEFF_FORCEINLINE const V &at(const K &key) const {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.at(*(QueryPair *)pair).value;
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		struct Iterator {
			typename SetType::Iterator _iterator;
			PEFF_FORCEINLINE Iterator(typename SetType::Iterator &&iteratorIn) : _iterator(iteratorIn) {
			}
			Iterator(const Iterator &rhs) = default;
			Iterator(Iterator &&rhs) = default;
			Iterator &operator=(const Iterator &rhs) = default;
			Iterator &operator=(Iterator &&rhs) = default;

			PEFF_FORCEINLINE bool operator==(const Iterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(Iterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++*this;
				return it;
			}

			PEFF_FORCEINLINE K &key() const {
				return _iterator->key;
			}

			PEFF_FORCEINLINE V &value() const {
				return _iterator->value;
			}

			PEFF_FORCEINLINE std::pair<K &, V &> operator*() const {
				return { _iterator.key(), _iterator.value() };
			}
		};

		Iterator begin() {
			return Iterator(_set.begin());
		}
		Iterator end() {
			return Iterator(_set.end());
		}
		Iterator beginReversed() {
			return Iterator(_set.beginReversed());
		}
		Iterator endReversed() {
			return Iterator(_set.endReversed());
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
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE const K &key() const {
				return _iterator.key();
			}

			PEFF_FORCEINLINE const V &value() const {
				return _iterator.value();
			}

			PEFF_FORCEINLINE std::pair<const K &, const V &> operator*() const {
				return { _iterator.key(), _iterator.value() };
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
		PEFF_FORCEINLINE ConstIterator begin() const {
			return beginConst();
		}
		PEFF_FORCEINLINE ConstIterator end() const {
			return endConst();
		}
		PEFF_FORCEINLINE ConstIterator beginReversed() const {
			return beginConstReversed();
		}
		PEFF_FORCEINLINE ConstIterator endReversed() const {
			return endConstReversed();
		}

		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(key));
		}

		PEFF_FORCEINLINE Iterator find(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return Iterator(_set.find(*(QueryPair *)pair));
		}

		PEFF_FORCEINLINE size_t size() const {
			return _set.size();
		}

		PEFF_FORCEINLINE bool copy(ThisType &dest) const {
			constructAt<ThisType>(&dest, allocator());

			ScopeGuard clearDestGuard([&dest]() noexcept {
				dest.clear();
			});

			for (ConstIterator i = beginConst(); i != endConst(); ++i) {
				Uninitialized<K> copiedKey;
				Uninitialized<V> copiedValue;

				if (!copiedKey.copyFrom(i.key())) {
					return false;
				}

				if (!copiedValue.copyFrom(i.value())) {
					return false;
				}

				if (!dest.insert(copiedKey.release(), copiedValue.release())) {
					return false;
				}
			}

			clearDestGuard.release();

			return true;
		}

		PEFF_FORCEINLINE bool copyAssign(ThisType &dest) const {
			verifyAlloc(allocator(), dest.allocator());

			return true;
		}
	};
}

#endif
