#ifndef _PEFF_CONTAINERS_SET_H_
#define _PEFF_CONTAINERS_SET_H_

#include "tree.h"

namespace peff {
	template <typename T, typename Comparator, bool Fallible, bool IsThreeway>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> &&std::strict_weak_order<Comparator, T, T>)
	class SetImpl final {
	private:
		using Tree = std::conditional_t<Fallible, FallibleRBTree<T, Comparator, IsThreeway>, RBTree<T, Comparator, IsThreeway>>;
		Tree _tree;
		using ThisType = SetImpl<T, Comparator, Fallible, IsThreeway>;
	public:
		using RemoveResultType = typename std::conditional_t<Fallible, bool, void>;
		using ElementQueryResultType = typename std::conditional_t<Fallible, Option<T &>, T &>;
		using ConstElementQueryResultType = typename std::conditional_t<Fallible, Option<const T &>, const T &>;
		using ContainsResultType = typename std::conditional_t<Fallible, Option<bool>, bool>;

		using NodeType = typename Tree::NodeType;

		PEFF_FORCEINLINE SetImpl(Alloc *allocator, Comparator &&comparator = {}) : _tree(allocator, std::move(comparator)) {
		}
		PEFF_FORCEINLINE SetImpl(ThisType &&rhs) : _tree(std::move(rhs._tree)) {
		}
		PEFF_FORCEINLINE ~SetImpl() {
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&dest) noexcept {
			_tree = std::move(dest._tree);
			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&value) {
			typename Tree::Node *node = _tree.insert(std::move(value));

			if (!node)
				return false;

#ifndef _NDEBUG
			_tree.verify();
#endif

			return true;
		}

		PEFF_FORCEINLINE RemoveResultType remove(const T &key) {
			if constexpr (Fallible) {
				auto node = _tree.get(key);

				if (!node.hasValue())
					return false;

				if (auto v = node.value(); v)
					_tree.remove(node.value());

				return true;
			} else {
				auto node = _tree.get(key);

				if (node)
					_tree.remove(node);
			}
		}

		PEFF_FORCEINLINE void verify() {
			_tree.verify();
		}

		PEFF_FORCEINLINE size_t size() {
			return _tree.size();
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _tree.allocator();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) noexcept {
			_tree.replaceAllocator(rhs);
		}

		PEFF_FORCEINLINE Comparator &comparator() {
			return _tree.comparator();
		}

		PEFF_FORCEINLINE const Comparator &comparator() const {
			return _tree.comparator();
		}

		PEFF_FORCEINLINE void clear() {
			_tree.clear();
		}

		PEFF_FORCEINLINE ElementQueryResultType at(const T &key) {
			if constexpr (Fallible) {
				auto node = _tree.get(key);

				if (!node.hasValue())
					return NULL_OPTION;

				assert(node.value());

				return node.value()->value;
			} else {
				auto node = _tree.get(key);

				assert(node);

				return node->value;
			}
		}

		PEFF_FORCEINLINE ConstElementQueryResultType at(const T &key) const {
			if constexpr (Fallible) {
				auto node = _tree.get(key);

				if (!node.hasValue())
					return NULL_OPTION;

				assert(node.value());

				return node.value()->value;
			} else {
				auto node = _tree.get(key);

				assert(node);

				return node->value;
			}
		}

		struct Iterator {
			typename Tree::Iterator _iterator;
			PEFF_FORCEINLINE Iterator(typename Tree::Iterator &&iteratorIn) : _iterator(iteratorIn) {
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

			PEFF_FORCEINLINE Iterator next() {
				Iterator iterator = *this;

				return ++iterator;
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

			PEFF_FORCEINLINE T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE T *operator->() const {
				return &*_iterator;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator(_tree.begin());
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(_tree.end());
		}
		PEFF_FORCEINLINE Iterator beginReversed() {
			return Iterator(_tree.beginReversed());
		}
		PEFF_FORCEINLINE Iterator endReversed() {
			return Iterator(_tree.endReversed());
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

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator->() const {
				return *_iterator;
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

		PEFF_FORCEINLINE ContainsResultType contains(const T &key) const {
			if constexpr (Fallible) {
				auto node = _tree.get(key);

				if (!node.hasValue())
					return NULL_OPTION;

				return node.value();
			} else {
				return _tree.get(key);
			}
		}
		PEFF_FORCEINLINE ConstIterator find(const T &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}

		PEFF_FORCEINLINE Iterator find(const T &key) {
			if constexpr (Fallible) {
				if (auto node = _tree.get(key); node.hasValue()) {
					return Iterator(typename Tree::Iterator(node.value(), &_tree, IteratorDirection::Forward));
				}
				return _tree.end();
			} else {
				if (auto node = _tree.get(key); node) {
					return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
				}
				return _tree.end();
			}
		}

		PEFF_FORCEINLINE Iterator findMaxLteq(const T &key) {
			if (auto node = _tree.getMaxLteqNode(key); node) {
				return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE ConstIterator findMaxLteq(const T &key) const {
			return const_cast<ThisType *>(this)->findMaxLteq(key);
		}

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			_tree.remove(iterator._iterator);
		}
	};

	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false>
	using Set = SetImpl<T, Comparator, false, IsThreeway>;
	template <typename T, typename Comparator = FallibleLt<T>, bool IsThreeway = false>
	using FallibleSet = SetImpl<T, Comparator, true, IsThreeway>;
}

#endif
