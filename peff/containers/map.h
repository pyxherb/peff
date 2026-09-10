#ifndef _PEFF_CONTAINERS_MAP_H_
#define _PEFF_CONTAINERS_MAP_H_

#include "set.h"

namespace peff {
	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false>
	class Map final {
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

		using SetType = Set<Pair, PairComparator, IsThreeway>;

		SetType _set;

		using ThisType = Map<K, V, Lt, IsThreeway>;

	public:
		using NodeType = typename SetType::NodeType;

		PEFF_FORCEINLINE Map(PEFF_NONNULL Alloc *allocator, Lt &&comparator = {}) : _set(allocator, PairComparator(std::move(comparator))) {}
		PEFF_FORCEINLINE Map(ThisType &&rhs) : _set(std::move(rhs._set)) {
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			clear();
			_set = std::move(rhs._set);
			return *this;
		}

		/// @brief Insert a key-value pair to the map.
		/// @param key Key of the key-value pair to be inserted into the map, must be an rvalue.
		/// @param value Value of the key-value pair to be inserted into the map, must be an rvalue.
		/// @return true The key and value were inserted successfully.
		/// @return false Failed to insert the key and value due to out of memory.
		PEFF_FORCEINLINE bool insert(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);
			return _set.insert(std::move(pair));
		}

		/// @brief Remove a key-value pair using a key.
		/// @param key Key associated to the pair to be removed.
		/// @return Removed pair containing the key and the value stored in the map previously.
		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove(const K &key) {
			auto opt = _set.remove(QueryPair(&key));
			if (!opt.has_value())
				return peff::NULLOPT;
			auto pair = std::move(opt).value();
			return std::pair<K, V>{ pair.key.release(), pair.value.release() };
		}

		/// @brief Remove a key-value pair using an alternative-typed key.
		/// @param key Alternative-typed key associated to the pair to be removed.
		/// @return Removed pair containing the key and the value stored in the map previously.
		template <typename U>
		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove_alt(const U &key) {
			auto opt = _set.template remove_alt<U>(key);
			if (!opt.has_value())
				return peff::NULLOPT;
			auto pair = std::move(opt).value();
			return std::pair<K, V>{ pair.key.release(), pair.value.release() };
		}

		/// @brief Check if a key is already associated with a key-value pair.
		/// @param key Key of the pair to be checked.
		/// @return true Corresponding key-value pair is already contained in this map.
		/// @return false Corresponding key-value pair was not in this map.
		PEFF_FORCEINLINE bool contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		/// @brief Check if an alternative-typed key is already associated with a key-value pair.
		/// @param key Alternative-typed key of the pair to be checked.
		/// @return true Corresponding key-value pair is already contained in this map.
		/// @return false Corresponding key-value pair was not in this map.
		template <typename U>
		PEFF_FORCEINLINE bool contains_alt(const U &key) const {
			return _set.template contains_alt<U>(key);
		}

		/// @brief Get reference to the value that is associated to a key.
		/// @param key Key to the key-value pair.
		/// @return Reference to the value of the key-value pair in the map.
		PEFF_FORCEINLINE V &at(const K &key) {
			return _set.at(QueryPair(&key)).value.get();
		}

		/// @brief Get immutable reference to the value that is associated to a key.
		/// @param key Key to the key-value pair.
		/// @return Immutable reference to the value of the key-value pair in the map.
		PEFF_FORCEINLINE const V &at(const K &key) const {
			return _set.at(QueryPair(&key)).value.get();
		}

		/// @brief Get reference to the value that is associated to a key, using an alternative-typed key.
		/// @param key Key to the key-value pair.
		/// @return Reference to the value of the key-value pair in the map.
		template <typename U>
		PEFF_FORCEINLINE V &at_alt(const U &key) {
			return _set.template at_alt<U>(key).value.get();
		}

		/// @brief Get immutable reference to the value that is associated to a key, using an alternative-typed key.
		/// @param key Key to the key-value pair.
		/// @return Immutable reference to the value of the key-value pair in the map.
		template <typename U>
		PEFF_FORCEINLINE const V &at_alt(const U &key) const {
			return _set.template at_alt<U>(key).value.get();
		}

		/// @brief Get allocator of the container.
		/// @return Allocator of the container.
		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return _set.allocator();
		}

		/// @brief Replace allocator of the container.
		/// @param rhs Allocator for replacement.
		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
			_set.replace_allocator(rhs);
		}

		/// @brief Get comparator of the container.
		/// @return Reference to the comparator to the container.
		PEFF_FORCEINLINE Lt &comparator() {
			return _set.comparator().inner_cmp;
		}

		/// @brief Get immutable comparator of the container.
		/// @return Immutable reference to the comparator to the container.
		PEFF_FORCEINLINE const Lt &comparator() const {
			return _set.comparator().inner_cmp;
		}

		/// @brief Clear the map.
		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		/// @brief Get number of key-value pairs contained in the map.
		/// @return Number of key-value pairs contained in the map.
		PEFF_FORCEINLINE size_t size() const {
			return _set.size();
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return _set.empty();
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

		/// @brief Find a key-value pair associated to the given key.
		/// @param key Key to be found.
		/// @return Immutable iterator to the key-value pair, an end iterator will be returned if not found.
		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}

		/// @brief Find a key-value pair associated to the given alternative-typed key.
		/// @param key Alternative-typed key to be found.
		/// @return Immutable iterator to the key-value pair, an end iterator will be returned if not found.
		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_alt(const U &key) const {
			return const_cast<ThisType *>(this)->template find_alt<U>(key);
		}

		/// @brief Find a key-value pair associated to the given key.
		/// @param key Key to be found.
		/// @return Iterator to the key-value pair, an end iterator will be returned if not found.
		PEFF_FORCEINLINE Iterator find(const K &key) {
			return Iterator(_set.find(QueryPair(&key)));
		}

		/// @brief Find a key-value pair associated to the given alternative-typed key.
		/// @param key Alternative-typed key to be found.
		/// @return Iterator to the key-value pair, an end iterator will be returned if not found.
		template <typename U>
		PEFF_FORCEINLINE Iterator find_alt(const U &key) {
			return Iterator(_set.template find_alt<U>(key));
		}

		/// @brief Find the key-value pair that is with the greatest key which is less than the given key.
		/// @param key Key for query.
		/// @return Immutable iterator to the key-value pair, an end iterator will be returned if not found.
		PEFF_FORCEINLINE ConstIterator find_max_lteq(const K &key) const {
			return const_cast<ThisType *>(this)->find_max_lteq(key);
		}

		/// @brief Find the key-value pair that is with the greatest key which is less than the given alternative-typed key.
		/// @param key Alternative-typed key for query.
		/// @return Immutable iterator to the key-value pair, an end iterator will be returned if not found.
		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_max_lteq_alt(const U &key) const {
			return const_cast<ThisType *>(this)->find_max_lteq_alt<U>(key);
		}

		/// @brief Find the key-value pair that is with the greatest key which is less than the given key.
		/// @param key Key for query.
		/// @return Iterator to the key-value pair, an end iterator will be returned if not found.
		PEFF_FORCEINLINE Iterator find_max_lteq(const K &key) {
			return Iterator(_set.find_max_lteq(QueryPair(&key)));
		}

		/// @brief Find the key-value pair that is with the greatest key which is less than the given alternative-typed key.
		/// @param key Alternative-typed key for query.
		/// @return Iterator to the key-value pair, an end iterator will be returned if not found.
		template <typename U>
		PEFF_FORCEINLINE Iterator find_max_lteq_alt(const U &key) {
			return Iterator(_set.template find_max_lteq_alt<U>(key));
		}

		/// @brief Remove the key-value pair associated to the iterator from the map.
		/// @param iterator Iterator to be removed.
		/// @return Pair containing previously removed key and value.
		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove(const Iterator &iterator) {
			peff::Option<Pair> pair = _set.remove(iterator._iterator);
			if (pair.has_value()) {
				assert(((*pair).key_constructed && (*pair).value_constructed) ||
					   ((!(*pair).key_constructed) && (!(*pair).key_constructed)));
				if ((*pair).key_constructed) {
					(*pair).key_constructed = false;
					(*pair).value_constructed = false;
					return std::pair<K, V>{ std::move((*pair).key.get()), std::move((*pair).value.get()) };
				}
				return peff::NULLOPT;
			}
			return peff::NULLOPT;
		}
	};
}

#endif
