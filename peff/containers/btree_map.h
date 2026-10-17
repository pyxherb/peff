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
			if ((key_constructed = rhs.key_constructed)) {
				key = std::move(rhs.key.get());
				rhs.key_constructed = false;
			}
			if ((value_constructed = rhs.value_constructed)) {
				value = std::move(rhs.value.get());
				rhs.value_constructed = false;
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

	/// @brief An ordered map backed by a B-tree.
	/// @tparam K The type of the keys.
	/// @tparam V The type of the values.
	/// @tparam Lt The type of the comparator used to order the keys.
	/// @tparam IsThreeway Whether the comparator yields a three-way ordering.
	/// @tparam Degree The degree of the B-tree.
	/// @tparam Fallible Whether the comparator is allowed to fail. When enabled together with
	///         IsThreeway, the comparator must return peff::PartialOrd, and every operation that
	///         compares keys may fail with peff::Unordered, which is reported through the returned
	///         peff::Result. In that case the tree is left unmodified when the comparison fails.
	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false, size_t Degree = _calc_btree_default_node_deg<BTreeMapPair<K, V>>(), bool Fallible = false>
	class BTreeMap final {
	private:
		static_assert(std::is_move_constructible_v<K>, "The key must be move-constructible");
		static_assert(std::is_move_constructible_v<V>, "The value must be move-constructible");
		static_assert(!Fallible || IsThreeway,
			"The fallible mode of BTreeMap requires a three-way comparator that returns peff::PartialOrd");

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

		using SetType = BTreeSet<Pair, PairComparator, IsThreeway, Degree, Fallible>;

		SetType _set;

		using ThisType = BTreeMap<K, V, Lt, IsThreeway, Degree, Fallible>;

		/// @brief Convert an optionally removed pair into the public pair type.
		/// @param opt The optional pair produced by a removal.
		/// @return The removed key and value, or an empty option when nothing was removed.
		[[nodiscard]] PEFF_FORCEINLINE static Option<std::pair<K, V>> _into_public_pair(Option<Pair> &&opt) {
			if (!opt.has_value())
				return NULLOPT;

			Pair pair = std::move(opt).value();
			return std::pair<K, V>{ pair.key.release(), pair.value.release() };
		}

	public:
		using NodeType = typename SetType::Node;

		/// @brief The result of an operation that may fail to compare two keys. In the fallible mode
		///        it is a peff::Result whose error type is peff::Unordered, otherwise it is the plain
		///        value type of the operation.
		template <typename X>
		using Outcome = std::conditional_t<Fallible, Result<X, Unordered>, X>;

		/// @brief The result of a lookup that yields a reference to the value.
		using AtResult = std::conditional_t<Fallible, Result<V *, Unordered>, V &>;

		/// @brief The result of a lookup that yields an immutable reference to the value.
		using ConstAtResult = std::conditional_t<Fallible, Result<const V *, Unordered>, const V &>;

		PEFF_FORCEINLINE BTreeMap(PEFF_NONNULL Alloc *allocator, Lt &&comparator = {}) : _set(allocator, PairComparator(std::move(comparator))) {}
		PEFF_FORCEINLINE BTreeMap(ThisType &&rhs) : _set(std::move(rhs._set)) {
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			clear();
			_set = std::move(rhs._set);
			return *this;
		}

		/// @brief Insert a key-value pair into the map.
		/// @param key Key of the pair to be inserted, must be an rvalue.
		/// @param value Value of the pair to be inserted, must be an rvalue.
		/// @return true when the pair was inserted. On failure it is false (out of memory) or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		[[nodiscard]] PEFF_FORCEINLINE Outcome<bool> insert(K &&key, V &&value) {
			Pair pair = Pair(std::move(key), std::move(value), false);

			if constexpr (Fallible) {
				auto result = _set.insert(std::move(pair));

				if (result.is_error())
					return Unordered();

				return result.value() != nullptr;
			} else {
				return _set.insert(std::move(pair)) != nullptr;
			}
		}

		/// @brief Remove a key-value pair using a key.
		/// @param key Key associated to the pair to be removed.
		/// @return Removed pair containing the key and the value stored previously, an empty option
		///         when there is no such pair, or, in the fallible mode, an Unordered error when the
		///         comparison failed. In the latter two cases the map is left unmodified.
		PEFF_FORCEINLINE Outcome<Option<std::pair<K, V>>> remove(const K &key) {
			if constexpr (Fallible) {
				auto opt = _set.remove(QueryPair(&key));

				if (opt.is_error())
					return Unordered();

				return _into_public_pair(std::move(opt).value());
			} else {
				return _into_public_pair(_set.remove(QueryPair(&key)));
			}
		}

		/// @brief Remove a key-value pair using an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key associated to the pair to be removed.
		/// @return Removed pair containing the key and the value stored previously, an empty option
		///         when there is no such pair, or, in the fallible mode, an Unordered error when the
		///         comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Option<std::pair<K, V>>> remove_alt(const U &key) {
			if constexpr (Fallible) {
				auto opt = _set.template remove_alt<U>(key);

				if (opt.is_error())
					return Unordered();

				return _into_public_pair(std::move(opt).value());
			} else {
				return _into_public_pair(_set.template remove_alt<U>(key));
			}
		}

		/// @brief Check if a key is already associated with a key-value pair.
		/// @param key Key of the pair to be checked.
		/// @return true when the key is associated with a pair, false otherwise, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<bool> contains(const K &key) const {
			return _set.contains(QueryPair(&key));
		}

		/// @brief Check if an alternative-typed key is already associated with a key-value pair.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key of the pair to be checked.
		/// @return true when the key is associated with a pair, false otherwise, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<bool> contains_alt(const U &key) const {
			return _set.template contains_alt<U>(key);
		}

		/// @brief Get reference to the value that is associated to a key.
		/// @param key Key to the key-value pair.
		/// @return Reference to the value, or, in the fallible mode, an Unordered error when the
		///         comparison failed.
		PEFF_FORCEINLINE AtResult at(const K &key) {
			if constexpr (Fallible) {
				auto result = _set.at(QueryPair(&key));

				if (result.is_error())
					return Unordered();

				return &result.value()->value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		/// @brief Get immutable reference to the value that is associated to a key.
		/// @param key Key to the key-value pair.
		/// @return Immutable reference to the value, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		PEFF_FORCEINLINE ConstAtResult at(const K &key) const {
			if constexpr (Fallible) {
				auto result = _set.at(QueryPair(&key));

				if (result.is_error())
					return Unordered();

				return &result.value()->value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		/// @brief Get reference to the value that is associated to an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key to the key-value pair.
		/// @return Reference to the value, or, in the fallible mode, an Unordered error when the
		///         comparison failed.
		template <typename U>
		PEFF_FORCEINLINE AtResult at_alt(const U &key) {
			if constexpr (Fallible) {
				auto result = _set.template at_alt<U>(key);

				if (result.is_error())
					return Unordered();

				return &result.value()->value.get();
			} else {
				return _set.template at_alt<U>(key).value.get();
			}
		}

		/// @brief Get immutable reference to the value that is associated to an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key to the key-value pair.
		/// @return Immutable reference to the value, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE ConstAtResult at_alt(const U &key) const {
			if constexpr (Fallible) {
				auto result = _set.template at_alt<U>(key);

				if (result.is_error())
					return Unordered();

				return &result.value()->value.get();
			} else {
				return _set.template at_alt<U>(key).value.get();
			}
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

		/// @brief Verify the invariants of the B-tree.
		PEFF_FORCEINLINE void verify() const noexcept {
			_set.verify();
		}

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

			PEFF_FORCEINLINE Iterator prev() const {
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

			PEFF_FORCEINLINE Iterator next() const {
				Iterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE IteratorDirection direction() const noexcept {
				return _iterator.direction();
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
		/// @return Immutable iterator to the pair, an end iterator when it is not present, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<ConstIterator> find(const K &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->find(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->find(key));
			}
		}

		/// @brief Find a key-value pair associated to the given key.
		/// @param key Key to be found.
		/// @return Iterator to the pair, an end iterator when it is not present, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Iterator> find(const K &key) {
			if constexpr (Fallible) {
				auto found = _set.find(QueryPair(&key));

				if (found.is_error())
					return Unordered();

				return Iterator(std::move(found.value()));
			} else {
				return Iterator(_set.find(QueryPair(&key)));
			}
		}

		/// @brief Find a key-value pair associated to the given alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key to be found.
		/// @return Immutable iterator to the pair, an end iterator when it is not present, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->template find_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->template find_alt<U>(key));
			}
		}

		/// @brief Find a key-value pair associated to the given alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key to be found.
		/// @return Iterator to the pair, an end iterator when it is not present, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _set.template find_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return Iterator(std::move(found.value()));
			} else {
				return Iterator(_set.template find_alt<U>(key));
			}
		}

		/// @brief Find the pair with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Immutable iterator to the pair, an end iterator when there is none, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq(const K &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->find_max_lteq(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->find_max_lteq(key));
			}
		}

		/// @brief Find the pair with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Iterator to the pair, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq(const K &key) {
			if constexpr (Fallible) {
				auto found = _set.find_max_lteq(QueryPair(&key));

				if (found.is_error())
					return Unordered();

				return Iterator(std::move(found.value()));
			} else {
				return Iterator(_set.find_max_lteq(QueryPair(&key)));
			}
		}

		/// @brief Find the pair with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key for query.
		/// @return Immutable iterator to the pair, an end iterator when there is none, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->template find_max_lteq_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->template find_max_lteq_alt<U>(key));
			}
		}

		/// @brief Find the pair with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Alternative-typed key for query.
		/// @return Iterator to the pair, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _set.template find_max_lteq_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return Iterator(std::move(found.value()));
			} else {
				return Iterator(_set.template find_max_lteq_alt<U>(key));
			}
		}

		/// @brief Remove the key-value pair associated to the iterator from the map.
		/// @param iterator Iterator to be removed.
		/// @return Pair containing previously removed key and value.
		PEFF_FORCEINLINE peff::Option<std::pair<K, V>> remove(const Iterator &iterator) {
			return _into_public_pair(_set.remove(iterator._iterator));
		}
	};
}

#endif
