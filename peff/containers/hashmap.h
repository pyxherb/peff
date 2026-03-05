#ifndef _PEFF_CONTAINERS_HASHMAP_H_
#define _PEFF_CONTAINERS_HASHMAP_H_

#include "hashset.h"

namespace peff {
	template <typename K, typename V, typename Eq, typename Hasher, bool Fallible>
	PEFF_REQUIRES_CONCEPT(std::invocable<Eq, const K &, const K &>)
	class HashMapImpl final {
	private:
		static_assert(std::is_move_constructible_v<K>, "The key must be move-constructible");
		static_assert(std::is_move_constructible_v<V>, "The value must be move-constructible");
		struct Pair {
			Uninit<K> key;
			Uninit<V> value;
			bool keyConstructed;
			bool valueConstructed;
			bool isForQuery;

			PEFF_FORCEINLINE Pair() : keyConstructed(false), valueConstructed(false), isForQuery(true) {}
			PEFF_FORCEINLINE Pair(K &&key, V &&value, bool isForQuery) : key(std::move(key)), value(std::move(value)), isForQuery(isForQuery), keyConstructed(true), valueConstructed(true) {}
			PEFF_FORCEINLINE Pair(Pair &&rhs) noexcept: isForQuery(false) {
				if (rhs.keyConstructed) {
					key = std::move(rhs.key.get());
					rhs.keyConstructed = false;
					keyConstructed = true;
				}
				if (rhs.valueConstructed) {
					value = std::move(rhs.value.get());
					rhs.valueConstructed = false;
					valueConstructed = true;
				}
			}

			PEFF_FORCEINLINE ~Pair() {
				if (keyConstructed)
					key.destroy();
				if (valueConstructed)
					value.destroy();
			}
		};

		struct QueryPair : public Pair {
			const K *queryKey;

			PEFF_FORCEINLINE QueryPair(const K *queryKey): Pair(), queryKey(queryKey) {}
		};

		struct PairComparator {
			Eq eqComparator;

			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.isForQuery ? *((const QueryPair &)lhs).queryKey : lhs.key.get(),
						&r = rhs.isForQuery ? *((const QueryPair &)rhs).queryKey : rhs.key.get();
				return eqComparator(l, r);
			}
		};

		struct PairHasher {
			Hasher hasher;

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const Pair &pair) const {
				const K &k = pair.isForQuery ? *((const QueryPair &)pair).queryKey : pair.key.get();
				return hasher(k);
			}
		};

		PairComparator comparator;

		using SetType = std::conditional_t<Fallible, FallibleHashSet<Pair, PairComparator, PairHasher>, HashSet<Pair, PairComparator, PairHasher>>;

		SetType _set;

		using ThisType = HashMapImpl<K, V, Eq, Hasher, Fallible>;

	public:
		using RemoveResultType = typename SetType::RemoveResultType;
		using ElementQueryResultType = typename std::conditional_t<Fallible, Option<V &>, V &>;
		using ConstElementQueryResultType = typename std::conditional_t<Fallible, Option<const V &>, const V &>;
		using ContainsResultType = typename SetType::ContainsResultType;

		PEFF_FORCEINLINE HashMapImpl(Alloc *allocator) : _set(allocator) {}
		PEFF_FORCEINLINE HashMapImpl(ThisType &&rhs) : comparator(std::move(rhs.comparator)), _set(std::move(rhs._set)) {
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			clearAndShrink();

			comparator = std::move(rhs.comparator);
			_set = std::move(rhs._set);

			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertWithoutResizeBuckets(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insertWithoutResizeBuckets(std::move(pair));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insert(std::move(pair));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertAndFetchKeyWithoutResizeBuckets(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insertWithoutResizeBuckets(std::move(pair));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insertAndFetchKey(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insert(std::move(pair));
		}

		[[nodiscard]] PEFF_FORCEINLINE RemoveResultType remove(const K &key) {
			if constexpr (Fallible) {
				return _set.remove(QueryPair(&key));
			} else {
				_set.remove(QueryPair(&key));
			}
		}

		PEFF_FORCEINLINE ContainsResultType contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const K &key) {
			if constexpr (Fallible) {
				auto maybePair = _set.at(QueryPair(&key));

				if (!maybePair.hasValue())
					return NULL_OPTION;

				return maybePair.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			if constexpr (Fallible) {
				auto maybePair = _set.at(QueryPair(&key));

				if (!maybePair.hasValue())
					return NULL_OPTION;

				return maybePair.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) noexcept {
			_set.replaceAllocator(rhs);
		}

		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		PEFF_FORCEINLINE void clearAndShrink() {
			_set.clearAndShrink();
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
				return _iterator->key.get();
			}

			PEFF_FORCEINLINE V &value() const {
				return _iterator->value.get();
			}

			PEFF_FORCEINLINE std::pair<K &, V &> operator*() const {
				return { _iterator->key.get(), _iterator->value.get() };
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
			return Iterator(_set.find(QueryPair(&key)));
		}

		PEFF_FORCEINLINE size_t size() const {
			return _set.size();
		}

		PEFF_FORCEINLINE bool shrinkBuckets() {
			return _set.shrinkBuckets();
		}
	};

	template <typename K, typename V, typename Eq = std::equal_to<K>, typename Hasher = peff::Hasher<K>>
	using HashMap = HashMapImpl<K, V, Eq, Hasher, false>;
	template <typename K, typename V, typename Eq = std::equal_to<K>, typename Hasher = peff::Hasher<K>>
	using FallibleHashMap = HashMapImpl<K, V, Eq, Hasher, true>;
}

#endif
