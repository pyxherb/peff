#ifndef _PHUL_CONTAINERS_TREE_H_
#define _PHUL_CONTAINERS_TREE_H_

#include <cassert>
#include <stdexcept>
#include <memory>
#include <memory_resource>
#include <type_traits>

#include "basedefs.h"
#include "misc.h"
#include <phul/base/allocator.h>
#include <phul/utils/misc.h>
#include <phul/utils/comparator.h>
#include <phul/utils/scope_guard.h>
#include "list.h"

namespace phul {
	enum class RBColor {
		Black = 0,
		Red = 1
	};

	class RBTreeBase {
	protected:
		struct AbstractNode {
			AbstractNode *p = nullptr, *l = nullptr, *r = nullptr;
			RBColor color = RBColor::Black;

			PHUL_CONTAINERS_API virtual ~AbstractNode();
		};

		AbstractNode *_root = nullptr;
		AbstractNode *_cachedMinNode = nullptr, *_cachedMaxNode = nullptr;
		size_t _nNodes = 0;

		PHUL_CONTAINERS_API static AbstractNode *_getMinNode(AbstractNode *node);
		PHUL_CONTAINERS_API static AbstractNode *_getMaxNode(AbstractNode *node);

		PHUL_FORCEINLINE static bool _isRed(AbstractNode *node) { return node && node->color == RBColor::Red; }
		PHUL_FORCEINLINE static bool _isBlack(AbstractNode *node) { return (!node) || node->color == RBColor::Black; }

		PHUL_CONTAINERS_API void _lRot(AbstractNode *x);
		PHUL_CONTAINERS_API void _rRot(AbstractNode *x);

		PHUL_CONTAINERS_API void _insertFixUp(AbstractNode *node);

		PHUL_CONTAINERS_API AbstractNode *_removeFixUp(AbstractNode *node);

		PHUL_CONTAINERS_API void _verify(AbstractNode *node, const size_t nBlack, size_t cntBlack) const;
		PHUL_CONTAINERS_API void _verify() const;

		PHUL_CONTAINERS_API static AbstractNode *_getNextNode(const AbstractNode *node, const AbstractNode *lastNode) noexcept;
		PHUL_CONTAINERS_API static AbstractNode *_getPrevNode(const AbstractNode *node, const AbstractNode *firstNode) noexcept;

		PHUL_CONTAINERS_API RBTreeBase();
		PHUL_CONTAINERS_API virtual ~RBTreeBase();
	};

	template <typename T,
		typename Comparator = LtComparator<T>,
		typename Allocator = StdAlloc>
	class RBTree : private RBTreeBase {
	public:
		struct Node : public RBTreeBase::AbstractNode {
			T value;

			inline Node() {}
			inline Node(T &&value) : value(value) {}
			virtual ~Node() {}
		};

	private:
		using ThisType = RBTree<T>;

		Comparator _comparator;
		Allocator _allocator;

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocSingleNode() {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator.release(node);
				});
			new (node) Node();

			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocSingleNode(const T &value) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator.release(node);
				});
			new (node) Node();
			if (!phul::copy(node->value, value)) {
				return false;
			}

			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocSingleNode(T &&value) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator.release(node);
				});
			new (node) Node(std::move(value));

			return node;
		}

		PHUL_FORCEINLINE void _deleteSingleNode(Node *node) {
			std::destroy_at<Node>(node);
			_allocator.release(node);
		}

		PHUL_FORCEINLINE void _deleteNodeTree(Node *node) {
			Node *maxNode = (Node *)_getMaxNode(node);
			Node *curNode = (Node *)_getMinNode(node);
			Node *parent = (Node*)node->p;
			bool walkedRootNode = false;

			while (curNode != parent) {
				if (curNode->r) {
					curNode = (Node *)_getMinNode(curNode->r);
				} else {
					Node *nodeToDelete = curNode;

					while (curNode->p && (curNode == curNode->p->r)) {
						nodeToDelete = curNode;
						curNode = (Node *)curNode->p;
						_deleteSingleNode(nodeToDelete);
					}

					nodeToDelete = curNode;
					curNode = (Node *)curNode->p;
					_deleteSingleNode(nodeToDelete);
				}
			}
		}

		struct CopyInfo {
			const Node *node;
			Node *newNode;
			bool isLeftWalked;
			bool isRightWalked;

			PHUL_FORCEINLINE bool copy(CopyInfo &dest) const {
				dest.node = node;
				dest.newNode = newNode;
				dest.isLeftWalked = isLeftWalked;
				dest.isRightWalked = isRightWalked;
				return true;
			}
		};

		PHUL_FORCEINLINE Node *_copyTree(const Node *node) {
			List<CopyInfo> copyInfoStack;

			Node *newNode = _allocSingleNode(node->value);
			if (!newNode)
				return nullptr;

			if (!copyInfoStack.pushBack(
					CopyInfo{ node,
						newNode,
						false }))
				return nullptr;

			while (copyInfoStack.getSize()) {
				CopyInfo &copyInfo = copyInfoStack.back();

				if ((!copyInfo.isLeftWalked)) {
					copyInfo.isLeftWalked = true;
					if (copyInfo.node->l) {
						Node *newNode = _allocSingleNode(((Node *)copyInfo.node->l)->value);
						newNode->p = copyInfo.newNode;
						copyInfo.newNode->l = newNode;
						if (!copyInfoStack.pushBack(
								CopyInfo{ ((Node *)copyInfo.node->l),
									newNode,
									false }))
							return false;
					}
				} else if (!(copyInfo.isRightWalked)) {
					copyInfo.isRightWalked = true;
					if (copyInfo.node->r) {
						Node *newNode = _allocSingleNode(((Node *)copyInfo.node->r)->value);
						newNode->p = copyInfo.newNode;
						copyInfo.newNode->r = newNode;
						if (!copyInfoStack.pushBack(
								CopyInfo{ ((Node *)copyInfo.node->r),
									newNode,
									false }))
							return false;
					}
				} else
					copyInfoStack.remove(copyInfoStack.lastNode());
			}

			return newNode;
		}

		PHUL_FORCEINLINE Node *_get(const T &key) {
			Node *i = (Node *)_root;
			while (i) {
				if (_comparator(i->value, key))
					i = (Node *)i->r;
				else if (_comparator(key, i->value))
					i = (Node *)i->l;
				else
					return i;
			}
			return nullptr;
		}

		PHUL_FORCEINLINE Node **_getSlot(const T &key, Node *&parentOut) {
			Node **i = (Node **)&_root;
			while (*i) {
				parentOut = *i;

				if (_comparator((*i)->value, key))
					i = (Node **)&((*i)->r);
				else if (_comparator(key, (*i)->value))
					i = (Node **)&((*i)->l);
				else
					return nullptr;
			}
			return i;
		}

		PHUL_FORCEINLINE void _insert(Node **slot, Node *parent, Node *node) {
			assert(!node->l);
			assert(!node->r);

			if (!_root) {
				_root = node;
				node->color = RBColor::Black;
				goto updateNodeCaches;
			}

			{
				if (_comparator(node->value, parent->value))
					parent->l = node;
				else
					parent->r = node;
				node->p = parent;
				node->color = RBColor::Red;

				_insertFixUp(node);
			}

		updateNodeCaches:
			_cachedMinNode = _getMinNode(_root);
			_cachedMaxNode = _getMaxNode(_root);

			++_nNodes;
		}

		PHUL_FORCEINLINE void _remove(Node *node) {
			AbstractNode *y = _removeFixUp(node);
			_deleteSingleNode((Node *)y);

			_cachedMinNode = _getMinNode(_root);
			_cachedMaxNode = _getMaxNode(_root);

			--_nNodes;
		}

	public:
		PHUL_FORCEINLINE RBTree() {}

		PHUL_FORCEINLINE bool copy(ThisType &dest) {
			if (!phul::copy(dest._allocator, _allocator))
				return false;

			ScopeGuard destroyAllocatorGuard([&dest]() {
				std::destroy_at<Allocator>(&dest._allocator);
			});

			if (!phul::copy(dest._comparator, _comparator))
				return false;

			ScopeGuard destroyComparatorGuard([&dest]() {
				std::destroy_at<Comparator>(&dest._comparator);
			});

			if (_root) {
				if (!(dest._root = _copyTree((Node *)_root)))
					return false;
			}
			else {
				dest._root = nullptr;
			}
			dest._cachedMinNode = _getMinNode(dest._root);
			dest._cachedMaxNode = _getMaxNode(dest._root);
			dest._nNodes = _nNodes;

			destroyAllocatorGuard.release();
			destroyComparatorGuard.release();
		}

		PHUL_FORCEINLINE bool copyAssign(ThisType &dest) {
			if (!phul::copyAssign(dest._allocator, _allocator))
				return false;

			ScopeGuard destroyAllocatorGuard([&dest]() {
				std::destroy_at<Allocator>(&dest._allocator);
			});

			if (!phul::copyAssign(dest._comparator, _comparator))
				return false;

			ScopeGuard destroyComparatorGuard([&dest]() {
				std::destroy_at<Comparator>(&dest._comparator);
			});

			if (!(dest._root = _copyTree((Node *)_root)))
				return false;
			dest._cachedMinNode = _getMinNode(dest._root);
			dest._cachedMaxNode = _getMaxNode(dest._root);
			dest._nNodes = _nNodes;

			destroyAllocatorGuard.release();
			destroyComparatorGuard.release();
		}

		PHUL_FORCEINLINE RBTree(ThisType &&other) {
			_root = other._root;
			_cachedMinNode = other._cachedMinNode;
			_cachedMaxNode = other._cachedMaxNode;
			_nNodes = other._nNodes;
			_comparator = std::move(other._comparator);
			_allocator = std::move(other._allocator);

			other._root = nullptr;
			other._cachedMinNode = nullptr;
			other._cachedMaxNode = nullptr;
			other._nNodes = 0;
		}

		virtual inline ~RBTree() {
			if (_root)
				_deleteNodeTree((Node *)_root);
		}

		PHUL_FORCEINLINE Node *get(const T &key) {
			return _get(key);
		}

		PHUL_FORCEINLINE const Node *get(const T &key) const {
			return const_cast<ThisType *>(this)->_get(key);
		}

		PHUL_FORCEINLINE Node *get(T &&key) {
			T k = key;
			return get(k);
		}

		PHUL_FORCEINLINE const Node *get(T &&key) const {
			T k = key;
			return get(k);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return Whether the node is inserted successfully, false if node with the same key presents.
		[[nodiscard]] PHUL_FORCEINLINE bool insert(Node *node) {
			Node *parent = nullptr, **slot = _getSlot(node->value, parent);

			if (!slot)
				return false;

			_insert(slot, parent, node);

			return true;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *insert(const T &key) {
			Node *parent = nullptr, **slot = _getSlot(key, parent);

			if (!slot)
				return parent;

			Node *node = _allocSingleNode(key);
			if (!node)
				return nullptr;
			if (!insert(node))
				_deleteSingleNode(node);

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *insert(T &&key) {
			Node *parent = nullptr, **slot = _getSlot(key, parent);

			if (!slot)
				return parent;

			Node *node = _allocSingleNode(key);
			if (!node)
				return nullptr;
			if (!insert(node))
				_deleteSingleNode(node);

			return node;
		}

		PHUL_FORCEINLINE void remove(Node *node) {
			_remove(node);
		}

		PHUL_FORCEINLINE void remove(const T &key) {
			remove(get(key));
		}

		PHUL_FORCEINLINE void remove(T &&key) {
			remove(get(key));
		}

		PHUL_FORCEINLINE void clear() {
			if (_root) {
				_deleteNodeTree((Node *)_root);
				_root = nullptr;
				_nNodes = 0;
			}
		}

		PHUL_FORCEINLINE void verify() {
			_verify();
		}

		static Node *getNextNode(const Node *node, const Node *lastNode) noexcept {
			return (Node *)_getNextNode((const AbstractNode *)node, (const AbstractNode *)lastNode);
		}

		static Node *getPrevNode(const Node *node, const Node *firstNode) noexcept {
			return (Node *)_getPrevNode((const AbstractNode *)node, (const AbstractNode *)firstNode);
		}

		struct Iterator {
			Node *node;
			ThisType *tree;
			IteratorDirection direction;

			PHUL_FORCEINLINE Iterator(
				Node *node,
				ThisType *tree,
				IteratorDirection direction)
				: node(node),
				  tree(tree),
				  direction(direction) {}

			PHUL_FORCEINLINE Iterator(const Iterator &it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;
			}
			PHUL_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;

				it.node = nullptr;
				it.tree = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PHUL_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}
			PHUL_FORCEINLINE Iterator &operator=(const Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}

			PHUL_FORCEINLINE bool copy(Iterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PHUL_FORCEINLINE Iterator &operator++() {
				if (!node)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					node = RBTree<T>::getNextNode(node, nullptr);
				} else {
					node = RBTree<T>::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PHUL_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++(*this);
				return it;
			}

			PHUL_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (node == tree->_cachedMinNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = RBTree<T>::getNextNode(node, nullptr);
				} else {
					if (node == tree->_cachedMaxNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = RBTree<T>::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PHUL_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PHUL_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PHUL_FORCEINLINE bool operator==(const Iterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node == it.node;
			}

			PHUL_FORCEINLINE bool operator==(const Iterator &&rhs) const {
				const Iterator it = rhs;
				return *this == it;
			}

			PHUL_FORCEINLINE bool operator!=(const Node *node) const noexcept {
				return node != node;
			}

			PHUL_FORCEINLINE bool operator!=(const Iterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node != it.node;
			}

			PHUL_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PHUL_FORCEINLINE T &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PHUL_FORCEINLINE T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PHUL_FORCEINLINE T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}

			PHUL_FORCEINLINE T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}
		};

		PHUL_FORCEINLINE Iterator begin() {
			return Iterator((Node *)_cachedMinNode, this, IteratorDirection::Forward);
		}
		PHUL_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PHUL_FORCEINLINE Iterator beginReversed() {
			return Iterator((Node *)_cachedMinNode, this, IteratorDirection::Reversed);
		}
		PHUL_FORCEINLINE Iterator endReversed() {
			return Iterator(nullptr, this, IteratorDirection::Reversed);
		}

		struct ConstIterator {
			const Node *node;
			const RBTree<T> *tree;
			IteratorDirection direction;

			PHUL_FORCEINLINE ConstIterator(
				const Node *node,
				const RBTree<T> *tree,
				IteratorDirection direction)
				: node(node),
				  tree(tree),
				  direction(direction) {}

			PHUL_FORCEINLINE ConstIterator(const ConstIterator &it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;
			}
			PHUL_FORCEINLINE ConstIterator(ConstIterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;

				it.node = nullptr;
				it.tree = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PHUL_FORCEINLINE ConstIterator &operator=(const ConstIterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				new (this) ConstIterator(rhs);
				return *this;
			}
			PHUL_FORCEINLINE ConstIterator &operator=(ConstIterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				new (this) ConstIterator(rhs);
				return *this;
			}

			PHUL_FORCEINLINE ConstIterator(const Iterator &it) {
				(*this) = it;
			}
			PHUL_FORCEINLINE ConstIterator(Iterator &&it) {
				(*this) = it;
			}
			PHUL_FORCEINLINE ConstIterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}
			PHUL_FORCEINLINE ConstIterator &operator=(Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}

			PHUL_FORCEINLINE bool copy(ConstIterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PHUL_FORCEINLINE ConstIterator &operator++() {
				if (!node)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					node = RBTree<T>::getNextNode(node, nullptr);
				} else {
					node = RBTree<T>::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PHUL_FORCEINLINE ConstIterator operator++(int) {
				ConstIterator it = *this;
				++(*this);
				return it;
			}

			PHUL_FORCEINLINE ConstIterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (node == tree->_cachedMinNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = RBTree<T>::getNextNode(node, nullptr);
				} else {
					if (node == tree->_cachedMaxNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = RBTree<T>::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PHUL_FORCEINLINE ConstIterator operator--(int) {
				ConstIterator it = *this;
				--(*this);
				return it;
			}

			PHUL_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PHUL_FORCEINLINE bool operator==(const ConstIterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node == it.node;
			}

			PHUL_FORCEINLINE bool operator==(ConstIterator &&rhs) const {
				const ConstIterator it = rhs;
				return *this == it;
			}

			PHUL_FORCEINLINE bool operator!=(const Node *node) const noexcept {
				return node != node;
			}

			PHUL_FORCEINLINE bool operator!=(const ConstIterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node != it.node;
			}

			PHUL_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				ConstIterator it = rhs;
				return *this != it;
			}

			PHUL_FORCEINLINE const T &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PHUL_FORCEINLINE const T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PHUL_FORCEINLINE const T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}

			PHUL_FORCEINLINE const T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}
		};

		PHUL_FORCEINLINE ConstIterator beginConst() const noexcept {
			return ConstIterator((Node *)_cachedMinNode, this, IteratorDirection::Forward);
		}
		PHUL_FORCEINLINE ConstIterator endConst() const noexcept {
			return ConstIterator(nullptr, this, IteratorDirection::Forward);
		}
		PHUL_FORCEINLINE ConstIterator beginConstReversed() const noexcept {
			return ConstIterator((Node *)_cachedMinNode, this, IteratorDirection::Reversed);
		}
		PHUL_FORCEINLINE ConstIterator endConstReversed() const noexcept {
			return ConstIterator(nullptr, this, IteratorDirection::Reversed);
		}
	};
}

#endif
