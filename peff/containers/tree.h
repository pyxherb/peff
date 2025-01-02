#ifndef _PEFF_CONTAINERS_TREE_H_
#define _PEFF_CONTAINERS_TREE_H_

#include <cassert>
#include <stdexcept>
#include <memory>
#include <memory_resource>
#include <type_traits>

#include "basedefs.h"
#include "misc.h"
#include <peff/base/allocator.h>
#include <peff/utils/misc.h>
#include <peff/utils/comparator.h>
#include <peff/utils/scope_guard.h>
#include "list.h"

namespace peff {
	enum class RBColor {
		Black = 0,
		Red = 1
	};

	class RBTreeBase {
	protected:
		struct AbstractNode {
			AbstractNode *p = nullptr, *l = nullptr, *r = nullptr;
			RBColor color = RBColor::Black;

			PEFF_CONTAINERS_API virtual ~AbstractNode();
		};

		AbstractNode *_root = nullptr;
		AbstractNode *_cachedMinNode = nullptr, *_cachedMaxNode = nullptr;
		size_t _nNodes = 0;

		PEFF_CONTAINERS_API static AbstractNode *_getMinNode(AbstractNode *node);
		PEFF_CONTAINERS_API static AbstractNode *_getMaxNode(AbstractNode *node);

		PEFF_FORCEINLINE static bool _isRed(AbstractNode *node) { return node && node->color == RBColor::Red; }
		PEFF_FORCEINLINE static bool _isBlack(AbstractNode *node) { return (!node) || node->color == RBColor::Black; }

		PEFF_CONTAINERS_API void _lRot(AbstractNode *x);
		PEFF_CONTAINERS_API void _rRot(AbstractNode *x);

		PEFF_CONTAINERS_API void _insertFixUp(AbstractNode *node);

		PEFF_CONTAINERS_API AbstractNode *_removeFixUp(AbstractNode *node);

		PEFF_CONTAINERS_API void _verify(AbstractNode *node, const size_t nBlack, size_t cntBlack) const;
		PEFF_CONTAINERS_API void _verify() const;

		PEFF_CONTAINERS_API static AbstractNode *_getNextNode(const AbstractNode *node, const AbstractNode *lastNode) noexcept;
		PEFF_CONTAINERS_API static AbstractNode *_getPrevNode(const AbstractNode *node, const AbstractNode *firstNode) noexcept;

		PEFF_CONTAINERS_API RBTreeBase();
		PEFF_CONTAINERS_API virtual ~RBTreeBase();
	};

	template <typename T,
		typename Comparator = LtComparator<T>>
	class RBTree : private RBTreeBase {
	public:
		struct Node : public RBTreeBase::AbstractNode {
			T value;

			inline Node() {}
			inline Node(T &&value) : value(value) {}
			virtual ~Node() {}
		};

	private:
		using ThisType = RBTree<T, Comparator>;

		Comparator _comparator;
		RcObjectPtr<Alloc> _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode() {
			Node *node = (Node *)_allocator->alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator->release(node);
				});
			new (node) Node();

			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode(const T &value) {
			Node *node = (Node *)_allocator->alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator->release(node);
				});
			new (node) Node();
			if (!peff::copy(node->value, value)) {
				return nullptr;
			}

			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode(T &&value) {
			Node *node = (Node *)_allocator->alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator->release(node);
				});
			new (node) Node(std::move(value));
			scopeGuard.release();

			return node;
		}

		PEFF_FORCEINLINE void _deleteSingleNode(Node *node) {
			std::destroy_at<Node>(node);
			_allocator->release(node);
		}

		PEFF_FORCEINLINE void _deleteNodeTree(Node *node) {
			Node *maxNode = (Node *)_getMaxNode(node);
			Node *curNode = (Node *)_getMinNode(node);
			Node *parent = (Node *)node->p;
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

			PEFF_FORCEINLINE bool copy(CopyInfo &dest) const {
				dest.node = node;
				dest.newNode = newNode;
				dest.isLeftWalked = isLeftWalked;
				dest.isRightWalked = isRightWalked;
				return true;
			}
		};

		PEFF_FORCEINLINE Node *_copyTree(const Node *node) {
			List<CopyInfo> copyInfoStack;

			Node *newNode = _allocSingleNode(node->value);
			if (!newNode)
				return nullptr;

			if (!copyInfoStack.pushBack(
					CopyInfo{ node,
						newNode,
						false }))
				return nullptr;

			ScopeGuard deleteNewTreeGuard([this, newNode]() {
				_deleteNodeTree(newNode);
			});

			while (copyInfoStack.size()) {
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
							return nullptr;
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
							return nullptr;
					}
				} else
					copyInfoStack.remove(copyInfoStack.lastNode());
			}

			return newNode;
		}

		PEFF_FORCEINLINE Node *_get(const T &key) {
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

		PEFF_FORCEINLINE Node **_getSlot(const T &key, Node *&parentOut) {
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

		PEFF_FORCEINLINE void _insert(Node **slot, Node *parent, Node *node) {
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

		PEFF_FORCEINLINE void _remove(Node *node) {
			AbstractNode *y = _removeFixUp(node);
			_deleteSingleNode((Node *)y);

			_cachedMinNode = _getMinNode(_root);
			_cachedMaxNode = _getMaxNode(_root);

			--_nNodes;
		}

	public:
		PEFF_FORCEINLINE RBTree(Alloc *allocator = getDefaultAlloc()) : _allocator(allocator) {}

		PEFF_FORCEINLINE bool copy(ThisType &dest) const {
			dest._allocator = _allocator;

			ScopeGuard destroyAllocatorGuard([&dest]() {
				dest._allocator->decRef();
			});

			if (!peff::copy(dest._comparator, _comparator))
				return false;

			ScopeGuard destroyComparatorGuard([&dest]() {
				std::destroy_at<Comparator>(&dest._comparator);
			});

			if (_root) {
				if (!(dest._root = const_cast<ThisType *>(this)->_copyTree((Node *)_root)))
					return false;
			} else {
				dest._root = nullptr;
			}
			dest._cachedMinNode = _getMinNode(dest._root);
			dest._cachedMaxNode = _getMaxNode(dest._root);
			dest._nNodes = _nNodes;

			destroyAllocatorGuard.release();
			destroyComparatorGuard.release();
		}

		PEFF_FORCEINLINE bool copyAssign(ThisType &dest) const {
			verifyAlloc(dest._allocator.get(), _allocator.get());

			ScopeGuard destroyAllocatorGuard([&dest]() {
				dest._allocator->decRef();
			});

			if (!peff::copyAssign(dest._comparator, _comparator))
				return false;

			ScopeGuard destroyComparatorGuard([&dest]() {
				std::destroy_at<Comparator>(&dest._comparator);
			});

			if (_root) {
				Node *backupRoot = (Node *)dest._root;
				if (!(dest._root = const_cast<ThisType *>(this)->_copyTree((Node *)_root))) {
					dest._root = backupRoot;
					return false;
				}
				dest._deleteNodeTree(backupRoot);
			} else {
				dest._root = nullptr;
			}
			dest._allocator = _allocator;
			dest._cachedMinNode = _getMinNode(dest._root);
			dest._cachedMaxNode = _getMaxNode(dest._root);
			dest._nNodes = _nNodes;

			destroyAllocatorGuard.release();
			destroyComparatorGuard.release();

			return true;
		}

		PEFF_FORCEINLINE RBTree(ThisType &&other) {
			_root = other._root;
			_cachedMinNode = other._cachedMinNode;
			_cachedMaxNode = other._cachedMaxNode;
			_nNodes = other._nNodes;
			_comparator = std::move(other._comparator);
			_allocator = other._allocator;

			other._root = nullptr;
			other._cachedMinNode = nullptr;
			other._cachedMaxNode = nullptr;
			other._nNodes = 0;
			other._allocator = nullptr;
		}

		virtual inline ~RBTree() {
			if (_root)
				_deleteNodeTree((Node *)_root);
		}

		PEFF_FORCEINLINE Node* getMaxLteqNode(const Node* node) {
			Node *curNode = (Node*)_root, *maxNode = NULL;

			while (curNode) {
				if (_comparator(curNode->value, node->value)) {
					maxNode = curNode;
					curNode = (Node*)curNode->r;
				} else if (_comparator(node->value, curNode->value)) {
					curNode = (Node *)curNode->l;
				} else
					return curNode;
			}

			return maxNode;
		}

		PEFF_FORCEINLINE Node *get(const T &key) {
			return _get(key);
		}

		PEFF_FORCEINLINE const Node *get(const T &key) const {
			return const_cast<ThisType *>(this)->_get(key);
		}

		PEFF_FORCEINLINE Node *get(T &&key) {
			T k = key;
			return get(k);
		}

		PEFF_FORCEINLINE const Node *get(T &&key) const {
			T k = key;
			return get(k);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return Whether the node is inserted successfully, false if node with the same key presents.
		[[nodiscard]] PEFF_FORCEINLINE bool insert(Node *node) {
			Node *parent = nullptr, **slot = _getSlot(node->value, parent);

			if (!slot)
				return false;

			_insert(slot, parent, node);

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insert(T &&key) {
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

		PEFF_FORCEINLINE void remove(Node *node) {
			_remove(node);
		}

		PEFF_FORCEINLINE void remove(const T &key) {
			remove(get(key));
		}

		PEFF_FORCEINLINE void remove(T &&key) {
			remove(get(key));
		}

		PEFF_FORCEINLINE void clear() {
			if (_root) {
				_deleteNodeTree((Node *)_root);
				_root = nullptr;
				_nNodes = 0;
			}
		}

		PEFF_FORCEINLINE void verify() {
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

			PEFF_FORCEINLINE Iterator(
				Node *node,
				ThisType *tree,
				IteratorDirection direction)
				: node(node),
				  tree(tree),
				  direction(direction) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;

				it.direction = IteratorDirection::Invalid;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}
			PEFF_FORCEINLINE Iterator &operator=(Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				new (this) Iterator(rhs);
				return *this;
			}

			PEFF_FORCEINLINE bool copy(Iterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				if (!node)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					node = ThisType::getNextNode(node, nullptr);
				} else {
					node = ThisType::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++(*this);
				return it;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (node == tree->_cachedMinNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = ThisType::getNextNode(node, nullptr);
				} else {
					if (node == tree->_cachedMaxNode)
						throw std::logic_error("Dereasing the begin iterator");

					node = ThisType::getPrevNode(node, nullptr);
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node == it.node;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &&rhs) const {
				const Iterator it = rhs;
				return *this == it;
			}

			PEFF_FORCEINLINE bool operator!=(const Node *node) const noexcept {
				return node != node;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node != it.node;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE T &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->value;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->value;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator((Node *)_cachedMinNode, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator beginReversed() {
			return Iterator((Node *)_cachedMinNode, this, IteratorDirection::Reversed);
		}
		PEFF_FORCEINLINE Iterator endReversed() {
			return Iterator(nullptr, this, IteratorDirection::Reversed);
		}

		struct ConstIterator {
			Iterator _iterator;

			PEFF_FORCEINLINE ConstIterator(
				Iterator &&iterator)
				: _iterator(iterator) {}

			ConstIterator(const ConstIterator &it) = default;
			PEFF_FORCEINLINE ConstIterator(ConstIterator &&it) : _iterator(std::move(it._iterator)) {
			}
			PEFF_FORCEINLINE ConstIterator &operator=(const ConstIterator &rhs) noexcept {
				_iterator = rhs._iterator;
				return *this;
			}
			PEFF_FORCEINLINE ConstIterator &operator=(ConstIterator &&rhs) noexcept {
				_iterator = std::move(rhs._iterator);
				return *this;
			}

			PEFF_FORCEINLINE bool copy(ConstIterator &dest) noexcept {
				new (&dest) ConstIterator(*this);
				return true;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator++(int) {
				ConstIterator it = *this;
				++(*this);
				return it;
			}

			PEFF_FORCEINLINE ConstIterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator--(int) {
				ConstIterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const ConstIterator &it) const {
				return _iterator == it._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &it) const {
				return _iterator != it._iterator;
			}

			PEFF_FORCEINLINE bool operator==(const Node *node) const {
				return _iterator == node;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				ConstIterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE const T &operator*() {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T *operator->() {
				return &*_iterator;
			}

			PEFF_FORCEINLINE const T *operator->() const {
				return &*_iterator;
			}
		};

		PEFF_FORCEINLINE ConstIterator beginConst() const noexcept {
			return ConstIterator(const_cast<ThisType*>(this)->begin());
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
	};
}

#endif
