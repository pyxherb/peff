#ifndef _PEFF_CONTAINERS_SET_H_
#define _PEFF_CONTAINERS_SET_H_

#include "tree.h"

namespace peff {
	template <typename T, typename Comparator = LtComparator<T>, typename Allocator = StdAlloc>
	class Set final {
	private:
		using Tree = RBTree<T, Comparator, Allocator>;
		Tree _tree;
		using ThisType = Set<T, Comparator, Allocator>;

	public:
		PEFF_FORCEINLINE ~Set() {
		}

		[[nodiscard]] PEFF_FORCEINLINE bool copy(ThisType& dest) const {
			return _tree.copy(dest._tree);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool copyAssign(ThisType &dest) const {
			return _tree.copyAssign(dest._tree);
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Tree::Node *insert(const T &value) {
			Tree::Node *node = _tree.insert(value);

			if (!node)
				return nullptr;

#ifndef NDEBUG
			_tree.verify();
#endif

			return node;
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Tree::Node *insert(T &&value) {
			Tree::Node *node = _tree.insert(value);

			if (!node)
				return nullptr;

#ifndef NDEBUG
			_tree.verify();
#endif

			return node;
		}

		PEFF_FORCEINLINE void remove(const T &key) {
			auto node = _tree.get(key);

			assert(node);

			_tree.remove(node);
		}

		PEFF_FORCEINLINE void remove(T &&key) {
			auto node = _tree.get(std::move(key));

			assert(node);

			_tree.remove(node);
		}

		PEFF_FORCEINLINE typename Tree::Node *find(const T &key) {
			return _tree.get(key);
		}

		PEFF_FORCEINLINE typename Tree::Node *find(T &&key) {
			return _tree.get(std::move(key));
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
