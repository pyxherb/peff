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

			PEFF_FORCEINLINE QueryPair(const K *queryKey) : Pair(), queryKey(queryKey) {}
		};

		struct PairComparator {
			Lt innerComparator;

			PEFF_FORCEINLINE PairComparator(Lt &&innerComparator) : innerComparator(std::move(innerComparator)) {}

			PEFF_FORCEINLINE decltype(std::declval<Lt>()(std::declval<K>(), std::declval<K>())) operator()(const Pair &lhs, const Pair &rhs) const {
				const K &l = lhs.isForQuery ? *((const QueryPair &)lhs).queryKey : lhs.key.get(),
						&r = rhs.isForQuery ? *((const QueryPair &)rhs).queryKey : rhs.key.get();
				return innerComparator(l, r);
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

				if (!v.hasValue())
					return NULL_OPTION;

				return v.value().value.get();
			} else {
				return _set.at(QueryPair(&key)).value.get();
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const K &key) const {
			if constexpr (Fallible) {
				auto v = _set.at(QueryPair(&key));

				if (!v.hasValue())
					return NULL_OPTION;

				return v.value().value.get();
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
			return Iterator(_set.find(QueryPair(&key)));
		}

		PEFF_FORCEINLINE ConstIterator findMaxLteq(const K &key) const {
			return const_cast<ThisType *>(this)->findMaxLteq(key);
		}

		PEFF_FORCEINLINE Iterator findMaxLteq(const K &key) {
			return Iterator(_set.findMaxLteq(QueryPair(&key)));
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
