#ifndef _PEFF_CONTAINERS_SET_H_
#define _PEFF_CONTAINERS_SET_H_

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

		PEFF_FORCEINLINE typename Tree::Iterator find(const T &key) {
			if (auto node = _tree.get(key); node) {
				return typename Tree::Iterator(node, &_tree, IteratorDirection::Forward);
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE void verify() {
			_tree.verify();
		}

		PEFF_FORCEINLINE typename Tree::Iterator begin() {
			return _tree.begin();
		}

		PEFF_FORCEINLINE typename Tree::Iterator end() {
			return _tree.end();
		}
	};
}

#endif
