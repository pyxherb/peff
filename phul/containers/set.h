#ifndef _PHUL_CONTAINERS_SET_H_
#define _PHUL_CONTAINERS_SET_H_

#include "tree.h"

namespace phul {
	template <typename T, typename Comparator = Comparator<T>, typename Allocator = StdAlloc>
	class Set final {
	private:
		using Tree = RBTree<T, Comparator, Allocator>;
		Tree _tree;

	public:
		PHUL_FORCEINLINE ~Set() {
		}

		[[nodiscard]] PHUL_FORCEINLINE typename Tree::Node *insert(const T &value) {
			Tree::Node *node = _tree.insert(value);

			if (!node)
				return nullptr;

#ifndef NDEBUG
			_tree.verify();
#endif

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE typename Tree::Node *insert(T &&value) {
			Tree::Node *node = _tree.insert(value);

			if (!node)
				return nullptr;

#ifndef NDEBUG
			_tree.verify();
#endif

			return node;
		}

		PHUL_FORCEINLINE void remove(const T &key) {
			auto node = _tree.get(key);

			assert(node);

			_tree.remove(node);
		}

		PHUL_FORCEINLINE void remove(T &&key) {
			auto node = _tree.get(key);

			assert(node);

			_tree.remove(node);
		}

		PHUL_FORCEINLINE void verify() {
			_tree.verify();
		}

		PHUL_FORCEINLINE typename Tree::Iterator begin() {
			return _tree.begin();
		}

		PHUL_FORCEINLINE typename Tree::Iterator end() {
			return _tree.end();
		}
	};
}

#endif
