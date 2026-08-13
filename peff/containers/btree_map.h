#ifndef _PEFF_CONTAINERS_BTREE_MAP_H_
#define _PEFF_CONTAINERS_BTREE_MAP_H_

#include "btree_set.h"

namespace peff {
	template <typename K, typename V>
	struct BTreeMapPair {
		Uninit<K> key;
		Uninit<V> value;
		bool key_constructed;
		bool value_constructed;
		bool for_query;

		PEFF_FORCEINLINE BTreeMapPair() : key_constructed(false), value_constructed(false), for_query(true) {}
		PEFF_FORCEINLINE BTreeMapPair(K &&key, V &&value, bool for_query) : key(std::move(key)), value(std::move(value)), for_query(for_query), key_constructed(true), value_constructed(true) {}
		PEFF_FORCEINLINE BTreeMapPair(BTreeMapPair<K, V> &&rhs) noexcept : for_query(false) {
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

		PEFF_FORCEINLINE ~BTreeMapPair() {
			if (key_constructed)
				key.destroy();
			if (value_constructed)
				value.destroy();
		}

		PEFF_FORCEINLINE BTreeMapPair<K, V> &operator=(BTreeMapPair<K, V> &&rhs) noexcept {
			if (key_constructed)
				key.destroy();
			if (value_constructed)
				value.destroy();
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
			return *this;
		}
	};

	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false, size_t Degree = _calc_btree_default_node_deg<BTreeMapPair<K, V>>()>
	class BTreeMap final {
	private:
		static_assert(std::is_move_constructible_v<K>, "The key must be move-constructible");
		static_assert(std::is_move_constructible_v<V>, "The value must be move-constructible");

		using Pair = BTreeMapPair<K, V>;

		/// @brief Extended pair used for query by key only.
		struct QueryPair : public Pair {
			const K *query_key;

			PEFF_FORCEINLINE QueryPair(PEFF_NONNULL const K *query_key) : Pair(), query_key(query_key) {}
		};

		/// @brief Wrapped comparator for comparing pairs by key.
		struct PairComparator {
			Lt inner_cmp;

			PEFF_FORCEINLINE PairComparator(Lt &&inner_cmp) : inner_cmp(std::move(inner_cmp)) {}

			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get(),
						&r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return inner_cmp(l, r);
			}

			template <typename U>
			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<K>(), std::declval<U>())) operator()(const Pair &lhs, const U &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get();
				return inner_cmp(l, rhs);
			}

			template <typename U>
			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<U>(), std::declval<K>())) operator()(const U &lhs, const Pair &rhs) const {
				const K &r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return inner_cmp(lhs, r);
			}
		};

		using SetType = BTreeSet<Pair, PairComparator, IsThreeway>;

		SetType _set;

		using ThisType = BTreeMap<K, V, Lt, IsThreeway, Degree>;

	public:
		using NodeType = typename SetType::Node;

		using ElementQueryResultType = V &;
		using ConstElementQueryResultType = const V &;

		PEFF_FORCEINLINE BTreeMap(PEFF_NONNULL Alloc *allocator, Lt &&comparator = {}) : _set(allocator, PairComparator(std::move(comparator))) {}
		PEFF_FORCEINLINE BTreeMap(ThisType &&rhs) : _set(std::move(rhs._set)) {
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			clear();
			_set = std::move(rhs._set);
			return *this;
		}

		PEFF_FORCEINLINE bool insert(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insert(std::move(pair));
		}

		PEFF_FORCEINLINE void remove(const K &key) {
			return _set.remove(QueryPair(&key));
		}

		PEFF_FORCEINLINE bool contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const K &key) {
			return _set.at(QueryPair(&key)).value.get();
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			return _set.at(QueryPair(&key)).value.get();
		}

		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
			_set.replace_allocator(rhs);
		}

		PEFF_FORCEINLINE Lt &comparator() {
			return _set.comparator().inner_cmp;
		}

		PEFF_FORCEINLINE const Lt &comparator() const {
			return _set.comparator().inner_cmp;
		}

		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		PEFF_FORCEINLINE size_t size() {
			return _set.size();
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

			PEFF_FORCEINLINE Iterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--*this;
				return it;
			}

			PEFF_FORCEINLINE Iterator prev() {
				Iterator iterator = *this;

				return --iterator;
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

			PEFF_FORCEINLINE Iterator next() {
				Iterator iterator = *this;

				return ++iterator;
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

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator(_set.begin());
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(_set.end());
		}
		PEFF_FORCEINLINE Iterator rbegin() {
			return Iterator(_set.rbegin());
		}
		PEFF_FORCEINLINE Iterator rend() {
			return Iterator(_set.rend());
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

			PEFF_FORCEINLINE ConstIterator next() {
				ConstIterator iterator = *this;

				return ++iterator;
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

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator cbegin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator cend() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator crbegin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rbegin());
		}
		PEFF_FORCEINLINE ConstIterator cend_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rend());
		}

		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}

		PEFF_FORCEINLINE Iterator find(const K &key) {
			return Iterator(_set.find(QueryPair(&key)));
		}

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			_set.remove(iterator._iterator);
		}
	};
}

#endif
