#ifndef _PEFF_CONTAINERS_tree_H_
#define _PEFF_CONTAINERS_tree_H_

#include "tree.h"

namespace peff {
	template <typename T, typename Comparator = LtComparator<T>>
	class Set final {
	private:
		using Tree = RBTree<T, Comparator>;
		Tree _tree;
		using ThisType = Set<T, Comparator>;

	public:
		PEFF_FORCEINLINE Set(Alloc *allocator = getDefaultAlloc()) : _tree(allocator) {
		}
		PEFF_FORCEINLINE ~Set() {
		}

		[[nodiscard]] PEFF_FORCEINLINE bool copy(ThisType& dest) const {
			return _tree.copy(dest._tree);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool copyAssign(ThisType &dest) const {
			return _tree.copyAssign(dest._tree);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&value) {
			typename Tree::Node *node = _tree.insert(std::move(value));

			if (!node)
				return false;

#ifndef NDEBUG
			_tree.verify();
#endif

			return true;
		}

		PEFF_FORCEINLINE void remove(const T &key) {
			auto node = _tree.get(key);

			assert(node);

			_tree.remove(node);
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

		PEFF_FORCEINLINE void clear() {
			_tree.clear();
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

			PEFF_FORCEINLINE T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE T *operator->() const {
				return &*_iterator;
			}
		};

		Iterator begin() {
			return Iterator(_tree.begin());
		}
		Iterator end() {
			return Iterator(_tree.end());
		}
		Iterator beginReversed() {
			return Iterator(_tree.beginReversed());
		}
		Iterator endReversed() {
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

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator->() const {
				return &*_iterator;
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

		PEFF_FORCEINLINE ConstIterator find(const T &key) const {
			if (auto node = _tree.get(key); node) {
				return ConstIterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE Iterator find(const T &key) {
			if (auto node = _tree.get(key); node) {
				return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			_tree.remove(iterator._iterator);
		}
	};
}

#endif
