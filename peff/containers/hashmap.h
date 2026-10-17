#ifndef _PEFF_CONTAINERS_HASHMAP_H_
#define _PEFF_CONTAINERS_HASHMAP_H_

#include "hashset.h"

namespace peff {
	template <typename K, typename V, typename Eq = std::equal_to<K>, typename Hasher = peff::Hasher<K>>
	PEFF_REQUIRES_CONCEPT(std::invocable<Eq, const K &, const K &>)
	class HashMap final {
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
			PEFF_FORCEINLINE Pair(Pair &&rhs) noexcept : for_query(false) {
				if ((key_constructed = rhs.key_constructed)) {
					key = std::move(rhs.key.get());
					rhs.key_constructed = false;
				}
				if ((value_constructed = rhs.value_constructed)) {
					value = std::move(rhs.value.get());
					rhs.value_constructed = false;
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

			PEFF_FORCEINLINE QueryPair(PEFF_NONNULL const K *query_key) : Pair(), query_key(query_key) {}
		};

		struct PairComparator {
			Eq eq_cmp;

			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get(),
						&r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return eq_cmp(l, r);
			}

			template <typename U>
			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<K>(), std::declval<U>())) operator()(const Pair &lhs, const U &rhs) const {
				const K &l = lhs.for_query ? *((const QueryPair &)lhs).query_key : lhs.key.get();
				return eq_cmp(l, rhs);
			}

			template <typename U>
			PEFF_FORCEINLINE decltype(std::declval<Eq>()(std::declval<U>(), std::declval<K>())) operator()(const U &lhs, const Pair &rhs) const {
				const K &r = rhs.for_query ? *((const QueryPair &)rhs).query_key : rhs.key.get();
				return eq_cmp(lhs, r);
			}
		};

		struct PairHasher {
			Hasher hasher;

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const Pair &pair) const {
				const K &k = pair.for_query ? *((const QueryPair &)pair).query_key : pair.key.get();
				return hasher(k);
			}

			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const QueryPair &pair) const {
				const K &k = *pair.query_key;
				return hasher(k);
			}

			template <typename U>
			PEFF_FORCEINLINE decltype(std::declval<Hasher>()(std::declval<K>())) operator()(const U &operand) const {
				return hasher(operand);
			}
		};

		PairComparator comparator;

		using SetType = HashSet<Pair, PairComparator, PairHasher>;

		SetType _set;

		using ThisType = HashMap<K, V, Eq, Hasher>;

	public:
		PEFF_FORCEINLINE HashMap(PEFF_NONNULL Alloc *allocator) : _set(allocator) {}
		PEFF_FORCEINLINE HashMap(ThisType &&rhs) : comparator(std::move(rhs.comparator)), _set(std::move(rhs._set)) {
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

		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove(const K &key) {
			auto opt = _set.remove(QueryPair(&key));
			if (!opt.has_value())
				return peff::NULLOPT;
			auto pair = std::move(opt).value();
			return std::pair<K, V>{ pair.key.release(), pair.value.release() };
		}

		template <typename U>
		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove_alt(const U &key) {
			auto opt = _set.template remove_alt<U>(key);
			if (!opt.has_value())
				return peff::NULLOPT;
			auto pair = std::move(opt).value();
			return std::pair<K, V>{ pair.key.release(), pair.value.release() };
		}

		[[nodiscard]] PEFF_FORCEINLINE bool contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE bool contains_alt(const U &key) const {
			return _set.template contains_alt<U>(key);
		}

		PEFF_FORCEINLINE V &at(const K &key) {
			return _set.at(QueryPair(&key)).value.get();
		}

		template <typename U>
		PEFF_FORCEINLINE V &at_alt(const U &key) {
			return _set.template at_alt<U>(key).value.get();
		}

		PEFF_FORCEINLINE const V &at(const K &key) const {
			return _set.at(QueryPair(&key)).value.get();
		}

		template <typename U>
		PEFF_FORCEINLINE const V &at_alt(const U &key) const {
			return _set.template at_alt<U>(key).value.get();
		}

		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
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

			PEFF_FORCEINLINE Iterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--*this;
				return it;
			}

			PEFF_FORCEINLINE Iterator next() const {
				Iterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE IteratorDirection direction() const noexcept {
				return _iterator.direction;
			}

			PEFF_FORCEINLINE Iterator prev() const {
				Iterator iterator = *this;

				return --iterator;
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
		Iterator rbegin() {
			return Iterator(_set.rbegin());
		}
		Iterator rend() {
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

			PEFF_FORCEINLINE ConstIterator operator++(int) {
				ConstIterator iterator = *this;
				++(*this);
				return iterator;
			}

			PEFF_FORCEINLINE ConstIterator next() const {
				ConstIterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE ConstIterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator--(int) {
				ConstIterator iterator = *this;
				--(*this);
				return iterator;
			}

			PEFF_FORCEINLINE ConstIterator prev() const {
				ConstIterator iterator = *this;

				return --iterator;
			}

			PEFF_FORCEINLINE IteratorDirection direction() const noexcept {
				return _iterator.direction();
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
		PEFF_FORCEINLINE ConstIterator begin() const {
			return cbegin();
		}
		PEFF_FORCEINLINE ConstIterator end() const {
			return cend();
		}
		PEFF_FORCEINLINE ConstIterator rbegin() const {
			return crbegin();
		}
		PEFF_FORCEINLINE ConstIterator rend() const {
			return cend_reversed();
		}

		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(key));
		}

		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_alt(const U &key) const {
			return ConstIterator(const_cast<ThisType *>(this)->find_alt<U>(key));
		}

		PEFF_FORCEINLINE Iterator find(const K &key) {
			return Iterator(_set.find(QueryPair(&key)));
		}

		template <typename U>
		PEFF_FORCEINLINE Iterator find_alt(const U &key) {
			return Iterator(_set.template find_alt<U>(key));
		}

		PEFF_FORCEINLINE size_t size() const {
			return _set.size();
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return _set.empty();
		}

		PEFF_FORCEINLINE bool shrink_buckets() {
			return _set.shrink_buckets();
		}
	};
}

#endif
