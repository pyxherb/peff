#ifndef _PEFF_CONTAINERS_MAP_H_
#define _PEFF_CONTAINERS_MAP_H_

#include "set.h"

namespace peff {
	template <typename K, typename V, typename Lt, bool Fallible, bool IsThreeway>
	class MapImpl final {
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

			PEFF_FORCEINLINE QueryPair(const K *query_key) : Pair(), query_key(query_key) {}
		};

		struct PairComparator {
			Lt inner_cmp;

			PEFF_FORCEINLINE PairComparator(Lt &&inner_cmp) : inner_cmp(std::move(inner_cmp)) {}

			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get(),
						&r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return inner_cmp(l, r);
			}
		};

		using SetType = std::conditional_t<Fallible, FallibleSet<Pair, PairComparator, IsThreeway>, Set<Pair, PairComparator, IsThreeway>>;

		SetType _set;

		using ThisType = MapImpl<K, V, Lt, Fallible, IsThreeway>;

	public:
		using NodeType = typename SetType::NodeType;

		using RemoveResultType = typename SetType::RemoveResultType;
		using ElementQueryResultType = typename std::conditional_t<Fallible, Option<V &>, V &>;
		using ConstElementQueryResultType = typename std::conditional_t<Fallible, Option<const V &>, const V &>;
		using ContainsResultType = typename SetType::ContainsResultType;

		PEFF_FORCEINLINE MapImpl(Alloc *allocator, Lt &&comparator = {}) : _set(allocator, PairComparator(std::move(comparator))) {}
		PEFF_FORCEINLINE MapImpl(ThisType &&rhs) : _set(std::move(rhs._set)) {
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

		PEFF_FORCEINLINE RemoveResultType remove(const K &key) {
			return _set.remove(QueryPair(&key));
		}

		PEFF_FORCEINLINE ContainsResultType contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const K &key) {
			if constexpr (Fallible) {
				auto v = _set.at(QueryPair(&key));

				if (!v.has_value())
					return NULL_OPTION;

				return v.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			if constexpr (Fallible) {
				auto v = _set.at(QueryPair(&key));

				if (!v.has_value())
					return NULL_OPTION;

				return v.value().value.get();
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

		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}

		PEFF_FORCEINLINE Iterator find(const K &key) {
			return Iterator(_set.find(QueryPair(&key)));
		}

		PEFF_FORCEINLINE ConstIterator find_max_lteq(const K &key) const {
			return const_cast<ThisType *>(this)->find_max_lteq(key);
		}

		PEFF_FORCEINLINE Iterator find_max_lteq(const K &key) {
			return Iterator(_set.find_max_lteq(QueryPair(&key)));
		}

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			_set.remove(iterator._iterator);
		}
	};

	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false>
	using Map = MapImpl<K, V, Lt, false, IsThreeway>;
	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false>
	using FallibleMap = MapImpl<K, V, Lt, true, IsThreeway>;
}

#endif
