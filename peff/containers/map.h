#ifndef _PEFF_CONTAINERS_MAP_H_
#define _PEFF_CONTAINERS_MAP_H_

#include "set.h"

namespace peff {
	template <typename K, typename V, typename Lt, bool Fallible, bool IsThreeway>
	class MapImpl final {
	private:
		struct Pair {
			K key;
			V value;
			bool isForQuery;

			PEFF_FORCEINLINE Pair(K &&k, V &&v, bool isForQuery = false) : key(std::move(k)), value(std::move(v)), isForQuery(isForQuery) {}
			Pair(Pair &&rhs) = default;
		};

		struct QueryPair : public Pair {
			const K *queryKey;
		};

		struct PairComparator {
			Lt innerComparator;

			PEFF_FORCEINLINE PairComparator(Lt &&innerComparator) : innerComparator(std::move(innerComparator)) {}

			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.isForQuery ? *((const QueryPair &)lhs).queryKey : lhs.key,
						&r = rhs.isForQuery ? *((const QueryPair &)rhs).queryKey : rhs.key;
				return innerComparator(l, r);
			}
		};

		using SetType = std::conditional_t<Fallible, FallibleSet<Pair, PairComparator, IsThreeway>, Set<Pair, PairComparator, IsThreeway>>;

		SetType _set;

		using ThisType = MapImpl<K, V, Lt, Fallible, IsThreeway>;

		PEFF_FORCEINLINE static void _constructKeyOnlyPairByCopy(const K &key, char *dest) {
			((QueryPair *)dest)->isForQuery = true;
			((QueryPair *)dest)->queryKey = &key;
		}

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
			Pair pair = Pair{ std::move(key), std::move(value), false };
			return _set.insert(std::move(pair));
		}

		PEFF_FORCEINLINE RemoveResultType remove(const K &key) {
			alignas(QueryPair) char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.remove(*(QueryPair *)pair);
		}

		PEFF_FORCEINLINE ContainsResultType contains(const K &key) const {
			alignas(QueryPair) char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return _set.contains(*(QueryPair *)pair);
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const K &key) {
			alignas(QueryPair) char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			if constexpr (Fallible) {
				auto v = _set.at(*(QueryPair *)pair);

				if (!v.hasValue())
					return NULL_OPTION;

				return v.value().value;
			} else {
				return _set.at(*(QueryPair *)pair).value;
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			alignas(QueryPair) char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			if constexpr (Fallible) {
				auto v = _set.at(*(QueryPair *)pair);

				if (!v.hasValue())
					return NULL_OPTION;

				return v.value().value;
			} else {
				return _set.at(*(QueryPair *)pair).value;
			}
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _set.allocator();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) noexcept {
			_set.replaceAllocator(rhs);
		}

		PEFF_FORCEINLINE Lt &comparator() {
			return _set.comparator().innerComparator;
		}

		PEFF_FORCEINLINE const Lt &comparator() const {
			return _set.comparator().innerComparator;
		}

		PEFF_FORCEINLINE void clear() {
			_set.clear();
		}

		PEFF_FORCEINLINE size_t size() {
			return _set.size();
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
				return _iterator->key;
			}

			PEFF_FORCEINLINE V &value() const {
				return _iterator->value;
			}

			PEFF_FORCEINLINE std::pair<K &, V &> operator*() const {
				return { _iterator->key, _iterator->value };
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

		PEFF_FORCEINLINE ConstIterator find(const K &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}

		PEFF_FORCEINLINE Iterator find(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return Iterator(_set.find(*(QueryPair *)pair));
		}

		PEFF_FORCEINLINE ConstIterator findMaxLteq(const K &key) const {
			return const_cast<ThisType *>(this)->findMaxLteq(key);
		}

		PEFF_FORCEINLINE Iterator findMaxLteq(const K &key) {
			char pair[sizeof(QueryPair)];

			_constructKeyOnlyPairByCopy(key, pair);

			return Iterator(_set.findMaxLteq(*(QueryPair *)pair));
		}

		PEFF_FORCEINLINE bool build(const std::initializer_list<std::pair<K, V>> &initializerList) noexcept {
			clear();

			ScopeGuard clearScopeGuard([this]() noexcept {
				clear();
			});

			for (auto &i : initializerList) {
				Uninitialized<K> copiedKey;
				Uninitialized<V> copiedValue;

				if (!copiedKey.copyFrom(i.first))
					return false;
				if (!copiedValue.copyFrom(i.second))
					return false;

				if (!insert(copiedKey.release(), copiedValue.release()))
					return false;
			}

			clearScopeGuard.release();

			return true;
		}

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			_set.remove(iterator._iterator);
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
	};

	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false>
	using Map = MapImpl<K, V, Lt, false, IsThreeway>;
	template <typename K, typename V, typename Lt = std::less<K>, bool IsThreeway = false>
	using FallibleMap = MapImpl<K, V, Lt, true, IsThreeway>;
}

#endif
