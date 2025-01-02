#ifndef _PEFF_CONTAINERS_HASHMAP_H_
#define _PEFF_CONTAINERS_HASHMAP_H_

#include "hashset.h"

namespace peff {
	template <typename K, typename V, typename Eq = EqComparator<K>, typename Hasher = Hasher<K>>
	class HashMap final {
	private:
		struct Pair {
			const K *queryKey;
			K key;
			V value;
		};

		struct PairComparator {
			Eq eqComparator;

			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<K>())) operator()(const Pair & lhs, const Pair & rhs) const {
				const K &l = lhs.queryKey ? *lhs.queryKey : lhs.key,
					&r = rhs.queryKey ? *rhs.queryKey : rhs.key;
				return eqComparator(l, r);
			}
		};

		struct PairHasher {
			Hasher hasher;

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const Pair & pair) const {
				const K &k = pair.queryKey ? *pair.queryKey : pair.key;
				return hasher(k);
			}
		};

		PairComparator comparator;

		using SetType = HashSet<Pair, PairComparator, PairHasher>;

		SetType _set;

		PEFF_FORCEINLINE static void _constructKeyOnlyPairByCopy(const K &key, char *dest) {
			((Pair *)dest)->queryKey = &key;
		}

	public:
		PEFF_FORCEINLINE HashMap(Alloc *allocator = getDefaultAlloc()) : _set(allocator) {}

		PEFF_FORCEINLINE bool insert(K&& key, V&& value) {
			return _set.insert(Pair{ nullptr, std::move(key), std::move(value) });
		}

		PEFF_FORCEINLINE bool remove(const K &key) {
			char pair[sizeof(Pair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.remove(*(Pair *)pair);
		}

		PEFF_FORCEINLINE bool contains(const K &key) const {
			char pair[sizeof(Pair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.contains(*(Pair *)pair);
		}

		PEFF_FORCEINLINE V &at(const K &key) {
			char pair[sizeof(Pair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.at(*(Pair *)pair).value;
		}

		PEFF_FORCEINLINE const V &at(const K &key) const {
			char pair[sizeof(Pair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.at(*(Pair *)pair).value;
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

			PEFF_FORCEINLINE Iterator& operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator &operator++(int) {
				Iterator it = *this;
				++*this;
				return it;
			}

			PEFF_FORCEINLINE K& key() {
				return _iterator->key;
			}

			PEFF_FORCEINLINE K &value() {
				return _iterator->value;
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
	};
}

#endif
