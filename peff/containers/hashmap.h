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
			bool key_constructed;
			bool value_constructed;
			bool for_query;

			PEFF_FORCEINLINE Pair() : key_constructed(false), value_constructed(false), for_query(true) {}
			PEFF_FORCEINLINE Pair(K &&key, V &&value, bool for_query) : key(std::move(key)), value(std::move(value)), for_query(for_query), key_constructed(true), value_constructed(true) {}
			PEFF_FORCEINLINE Pair(Pair &&rhs) noexcept: for_query(false) {
				if (rhs.key_constructed) {
					key = std::move(rhs.key.get());
					rhs.key_constructed = false;
					key_constructed = true;
				}
				if (rhs.value_constructed) {
					value = std::move(rhs.value.get());
					rhs.value_constructed = false;
					value_constructed = true;
				}
			}

			PEFF_FORCEINLINE ~Pair() {
				if (key_constructed)
					key.destroy();
				if (value_constructed)
					value.destroy();
			}
		};

		struct QueryPair : public Pair {
			const K *query_key;

			PEFF_FORCEINLINE QueryPair(const K *query_key): Pair(), query_key(query_key) {}
		};

		struct PairComparator {
			Eq eq_cmp;

			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get(),
						&r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return eq_cmp(l, r);
			}
		};

		struct PairHasher {
			Hasher hasher;

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const Pair &pair) const {
				const K &k = pair.for_query ? *((const QueryPair &)pair).query_key : pair.key.get();
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
			clear_and_shrink();

			comparator = std::move(rhs.comparator);
			_set = std::move(rhs._set);

			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert_without_resize_buckets(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insert_without_resize_buckets(std::move(pair));
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(K &&key, V &&value) {
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
				auto maybe_pair = _set.at(QueryPair(&key));

				if (!maybe_pair.has_value())
					return NULL_OPTION;

				return maybe_pair.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			if constexpr (Fallible) {
				auto maybe_pair = _set.at(QueryPair(&key));

				if (!maybe_pair.has_value())
					return NULL_OPTION;

				return maybe_pair.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) noexcept {
			_set.replace_allocator(rhs);
		}

		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		PEFF_FORCEINLINE void clear_and_shrink() {
			_set.clear_and_shrink();
		}

		struct Iterator {
			typename SetType::Iterator _iterator;
			PEFF_FORCEINLINE Iterator(typename SetType::Iterator &&iterator_in) : _iterator(iterator_in) {
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
		Iterator begin_reversed() {
			return Iterator(_set.begin_reversed());
		}
		Iterator end_reversed() {
			return Iterator(_set.end_reversed());
		}

		struct ConstIterator {
			Iterator _iterator;
			PEFF_FORCEINLINE ConstIterator(Iterator &&iterator_in) : _iterator(iterator_in) {
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

		PEFF_FORCEINLINE ConstIterator begin_const() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator end_const() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator begin_const_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin_reversed());
		}
		PEFF_FORCEINLINE ConstIterator end_const_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end_reversed());
		}
		PEFF_FORCEINLINE ConstIterator begin() const {
			return begin_const();
		}
		PEFF_FORCEINLINE ConstIterator end() const {
			return end_const();
		}
		PEFF_FORCEINLINE ConstIterator begin_reversed() const {
			return begin_const_reversed();
		}
		PEFF_FORCEINLINE ConstIterator end_reversed() const {
			return end_const_reversed();
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

		PEFF_FORCEINLINE bool shrink_buckets() {
			return _set.shrink_buckets();
		}
	};

	template <typename K, typename V, typename Eq = std::equal_to<K>, typename Hasher = peff::Hasher<K>>
	using HashMap = HashMapImpl<K, V, Eq, Hasher, false>;
	template <typename K, typename V, typename Eq = std::equal_to<K>, typename Hasher = peff::Hasher<K>>
	using FallibleHashMap = HashMapImpl<K, V, Eq, Hasher, true>;
}

#endif
