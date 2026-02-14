#ifndef _PEFF_CONTAINERS_RBTREE_H_
#define _PEFF_CONTAINERS_RBTREE_H_

#include <cassert>
#include <stdexcept>
#include <memory>
#include <memory_resource>
#include <type_traits>

#if __cplusplus >= 202002L
	#include <concepts>
#endif

#include "basedefs.h"
#include "misc.h"
#include <peff/base/alloc.h>
#include <peff/base/misc.h>
#include <peff/base/scope_guard.h>
#include <peff/utils/fallible_cmp.h>
#include <peff/utils/option.h>
#include <stdexcept>

namespace peff {
	enum class RBColor {
		Black = 0,
		Red = 1
	};

	class RBTreeBase {
	protected:
		struct NodeBase {
			NodeBase *p = nullptr, *l = nullptr, *r = nullptr;
			RBColor color = RBColor::Black;
		};

		NodeBase *_root = nullptr;
		NodeBase *_cachedMinNode = nullptr, *_cachedMaxNode = nullptr;
		size_t _nNodes = 0;

		PEFF_CONTAINERS_API static NodeBase *_getMinNode(NodeBase *node) noexcept;
		PEFF_CONTAINERS_API static NodeBase *_getMaxNode(NodeBase *node) noexcept;

		PEFF_FORCEINLINE static bool _isRed(NodeBase *node) noexcept { return node && node->color == RBColor::Red; }
		PEFF_FORCEINLINE static bool _isBlack(NodeBase *node) noexcept { return (!node) || node->color == RBColor::Black; }

		PEFF_CONTAINERS_API void _lRot(NodeBase *x) noexcept;
		PEFF_CONTAINERS_API void _rRot(NodeBase *x) noexcept;

		PEFF_CONTAINERS_API void _insertFixUp(NodeBase *node) noexcept;

		PEFF_CONTAINERS_API NodeBase *_removeFixUp(NodeBase *node) noexcept;

		PEFF_CONTAINERS_API void _verify(NodeBase *node, const size_t nBlack, size_t cntBlack) const noexcept;
		PEFF_CONTAINERS_API void _verify() const noexcept;

		PEFF_CONTAINERS_API static NodeBase *_getNextNode(const NodeBase *node, const NodeBase *lastNode) noexcept;
		PEFF_CONTAINERS_API static NodeBase *_getPrevNode(const NodeBase *node, const NodeBase *firstNode) noexcept;

		PEFF_CONTAINERS_API RBTreeBase() noexcept;
		PEFF_CONTAINERS_API ~RBTreeBase();
	};

	template <typename T,
		typename Comparator,
		bool Fallible,
		bool IsThreeway>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> &&std::strict_weak_order<Comparator, T, T>)
	class RBTreeImpl : protected RBTreeBase {
	public:
		struct Node : public RBTreeBase::NodeBase {
			T treeKey;

			PEFF_FORCEINLINE Node(T &&key) : treeKey(std::move(key)) {}
			PEFF_FORCEINLINE ~Node() {}
		};

		using NodeType = Node;
		using ComparatorType = Comparator;

	protected:
		using NodeQueryResultType = typename std::conditional<Fallible, Option<Node *>, Node *>::type;

		using ThisType = RBTreeImpl<T, Comparator, Fallible, IsThreeway>;

		Comparator _comparator;
		RcObjectPtr<Alloc> _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode(T &&value) {
			Node *node = (Node *)allocAndConstruct<Node>(_allocator.get(), alignof(Node), std::move(value));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _deleteSingleNode(Node *node) {
			destroyAndRelease<Node>(_allocator.get(), node, alignof(Node));
		}

		PEFF_FORCEINLINE void _deleteNodeTree(Node *node) {
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

		PEFF_FORCEINLINE NodeQueryResultType _get(const T &key) const {
			Node *i = (Node *)_root;

			if constexpr (Fallible) {
				while (i) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(i->treeKey, key);

						if (result.value() > 0)
							i = (Node *)i->r;
						else if (result.value() < 0)
							i = (Node *)i->l;
						else
							return i;
					} else {
						Option<bool> result;

						if ((result = _comparator(i->treeKey, key)).hasValue()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(key, i->treeKey)).hasValue()) {
									assert(!result.value());
									i = (Node *)i->r;
								} else {
									return NULL_OPTION;
								}
#else
								i = (Node *)i->r;
#endif
							} else if ((result = _comparator(key, i->treeKey)).hasValue()) {
								if (result.value()) {
									i = (Node *)i->l;
								} else
									return i;
							} else {
								return NULL_OPTION;
							}
						} else {
							return NULL_OPTION;
						}
					}
				}
			} else {
				while (i) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(i->treeKey, key);
						if (result > 0)
							i = (Node *)i->r;
						else if (result < 0)
							i = (Node *)i->l;
						else
							return i;
					} else {
						if (_comparator(i->treeKey, key)) {
							assert(!_comparator(key, i->treeKey));
							i = (Node *)i->r;
						} else if (_comparator(key, i->treeKey)) {
							i = (Node *)i->l;
						} else
							return i;
					}
				}
			}
			return nullptr;
		}

		PEFF_FORCEINLINE NodeBase **_getSlot(const T &key, NodeBase *&parentOut) {
			NodeBase **i = (NodeBase **)&_root;
			parentOut = nullptr;

			if constexpr (Fallible) {
				while (*i) {
					parentOut = *i;

					if constexpr (IsThreeway) {
						auto &&result = _comparator(static_cast<Node *>(*i)->treeKey, key);

						if (result.value() > 0)
							i = (NodeBase **)&static_cast<Node *>(*i)->r;
						else if (result.value() < 0)
							i = (NodeBase **)&static_cast<Node *>(*i)->l;
						else
							return i;
					} else {
						Option<bool> result;

						if ((result = _comparator(static_cast<Node *>(*i)->treeKey, key)).hasValue()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(key, static_cast<Node *>(*i)->treeKey)).hasValue()) {
									assert(!result.value());
									i = (NodeBase **)&static_cast<Node *>(*i)->r;
								} else {
									return nullptr;
								}
#else
								i = (NodeBase **)&static_cast<Node *>(*i)->r;
#endif
							} else if ((result = _comparator(key, static_cast<Node *>(*i)->treeKey)).hasValue()) {
								if (result.value()) {
									i = (NodeBase **)&static_cast<Node *>(*i)->l;
								} else
									return i;
							} else {
								return nullptr;
							}
						} else {
							return nullptr;
						}
					}
				}
			} else {
				if constexpr (IsThreeway) {
					while (*i) {
						parentOut = *i;

						auto &&result = _comparator(static_cast<Node *>(*i)->treeKey, key);

						if (result > 0) {
							i = (NodeBase **)&((*i)->r);
						} else if (result < 0) {
							i = (NodeBase **)&((*i)->l);
						} else
							return nullptr;
					}
				} else {
					while (*i) {
						parentOut = *i;

						if (_comparator(static_cast<Node *>(*i)->treeKey, key)) {
							assert(!_comparator(key, static_cast<Node *>(*i)->treeKey));
							i = (NodeBase **)&((*i)->r);
						} else if (_comparator(key, static_cast<Node *>(*i)->treeKey)) {
							i = (NodeBase **)&((*i)->l);
						} else
							return nullptr;
					}
				}
			}
			return i;
		}

		PEFF_FORCEINLINE bool _insert(Node *parent, Node *node) {
			assert(!node->l);
			assert(!node->r);

			if (!_root) {
				_root = node;
				node->color = RBColor::Black;
				goto updateNodeCaches;
			}

			if constexpr (Fallible) {
				if constexpr (IsThreeway) {
					if (auto &&result = _comparator(node->treeKey, parent->treeKey); result.hasValue()) {
						if (result.value() > 0)
							parent->l = node;
						else if (result.value() < 0)
							parent->r = node;
						else
							assert(false);
					} else {
						return false;
					}
				} else {
					Option<bool> result;
					if ((result = _comparator(node->treeKey, parent->treeKey)).hasValue()) {
						if (result.value())
							parent->l = node;
						else
							parent->r = node;
					} else {
						return false;
					}
				}
			} else {
				if (IsThreeway) {
					auto &&result = _comparator(node->treeKey, parent->treeKey);
					if (result > 0)
						parent->l = node;
					else if (result < 0)
						parent->r = node;
					else
						assert(false);
				} else {
					if (_comparator(node->treeKey, parent->treeKey))
						parent->l = node;
					else
						parent->r = node;
				}
			}
			node->p = parent;
			node->color = RBColor::Red;

			_insertFixUp(node);

		updateNodeCaches:
			_cachedMinNode = _getMinNode(_root);
			_cachedMaxNode = _getMaxNode(_root);

			++_nNodes;

			return true;
		}

		PEFF_FORCEINLINE Node *_remove(Node *node) {
			Node *y = (Node *)_removeFixUp(node);

			_cachedMinNode = _getMinNode(_root);
			_cachedMaxNode = _getMaxNode(_root);

			--_nNodes;

			return y;
		}

	public:
		PEFF_FORCEINLINE RBTreeImpl(Alloc *allocator, Comparator &&comparator) : _allocator(allocator), _comparator(std::move(comparator)) {}

		PEFF_FORCEINLINE RBTreeImpl(ThisType &&other)
			: _comparator(std::move(other._comparator)),
			  _allocator(std::move(other._allocator)) {
			_root = other._root;
			_cachedMinNode = other._cachedMinNode;
			_cachedMaxNode = other._cachedMaxNode;
			_nNodes = other._nNodes;

			other._root = nullptr;
			other._cachedMinNode = nullptr;
			other._cachedMaxNode = nullptr;
			other._nNodes = 0;
		}

		PEFF_FORCEINLINE ~RBTreeImpl() {
			if (_root)
				_deleteNodeTree((Node *)_root);
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) noexcept {
			verifyAlloc(other._allocator.get(), _allocator.get());

			clear();

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

			return *this;
		}

		PEFF_FORCEINLINE NodeQueryResultType getMaxLteqNode(const T &data) {
			Node *curNode = (Node *)_root, *maxNode = NULL;

			if constexpr (Fallible) {
				while (curNode) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(curNode->treeKey, data);

						if (result.value() > 0) {
							maxNode = curNode;
							curNode = (Node *)curNode->r;
						} else if (result.value() < 0)
							curNode = (Node *)curNode->l;
						else
							return curNode;
					} else {
						Option<bool> result;

						if ((result = _comparator(curNode->treeKey, data)).hasValue()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(data, curNode->treeKey)).hasValue()) {
									assert(!result.value());
									curNode = (Node *)curNode->r;
								} else {
									return NULL_OPTION;
								}
#else
								maxNode = curNode;
								curNode = (Node *)curNode->r;
#endif
							} else if ((result = _comparator(data, curNode->treeKey)).hasValue()) {
								if (result.value()) {
									curNode = (Node *)curNode->l;
								} else
									return curNode;
							} else {
								return NULL_OPTION;
							}
						} else {
							return NULL_OPTION;
						}
					}
				}
			} else {
				while (curNode) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(curNode->treeKey, data);
						if (result > 0) {
							maxNode = curNode;
							curNode = (Node *)curNode->r;
						} else if (result < 0)
							curNode = (Node *)curNode->l;
						else
							return curNode;
					} else {
						if (_comparator(curNode->treeKey, data)) {
							assert(!_comparator(data, curNode->treeKey));
							maxNode = curNode;
							curNode = (Node *)curNode->r;
						} else if (_comparator(data, curNode->treeKey)) {
							curNode = (Node *)curNode->l;
						} else
							return curNode;
					}
				}
			}

			return maxNode;
		}

		PEFF_FORCEINLINE NodeQueryResultType get(const T &key) const {
			return _get(key);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return Whether the node is inserted successfully, false if node with the same key presents.
		[[nodiscard]] PEFF_FORCEINLINE bool insert(Node *node) {
			NodeBase *parent = nullptr;
			NodeBase **slot = _getSlot(node->treeKey, parent);

			if (!slot)
				return false;

			return _insert(static_cast<Node *>(parent), node);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insert(T &&key) {
			NodeBase *parent = nullptr, **slot = _getSlot(key, parent);

			if (!slot)
				return static_cast<Node *>(parent);

			Node *node = _allocSingleNode(std::move(key));
			if (!node)
				return nullptr;
			if (!insert(node))
				_deleteSingleNode(node);

			return node;
		}

		PEFF_FORCEINLINE void remove(Node *node, bool deleteNode = true) {
			Node *y = _remove(node);
			if (deleteNode)
				_deleteSingleNode((Node *)y);
			else {
				y->l = nullptr;
				y->r = nullptr;
				y->p = nullptr;
			}
		}

		PEFF_FORCEINLINE void clear() {
			if (_root) {
				_deleteNodeTree((Node *)_root);
				_root = nullptr;
				_nNodes = 0;
			}
			_cachedMaxNode = nullptr;
			_cachedMinNode = nullptr;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _nNodes;
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return const_cast<ThisType *>(this)->_allocator.get();
		}

		PEFF_FORCEINLINE void replaceAllocator(Alloc *rhs) {
			verifyReplaceable(_allocator.get(), rhs);

			_allocator = rhs;
		}

		PEFF_FORCEINLINE Comparator &comparator() {
			return _comparator;
		}

		PEFF_FORCEINLINE const Comparator &comparator() const {
			return _comparator;
		}

		PEFF_FORCEINLINE void verify() {
			_verify();
		}

		static Node *getNextNode(const Node *node, const Node *lastNode) noexcept {
			return (Node *)_getNextNode((const NodeBase *)node, (const NodeBase *)lastNode);
		}

		static Node *getPrevNode(const Node *node, const Node *firstNode) noexcept {
			return (Node *)_getPrevNode((const NodeBase *)node, (const NodeBase *)firstNode);
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
				constructAt<Iterator>(this, std::move(rhs));
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

			PEFF_FORCEINLINE Iterator next() {
				Iterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (node == tree->_cachedMinNode)
						throw std::logic_error("Dereasing the begin iterator");

					if (!node)
						node = (Node *)tree->_cachedMaxNode;
					else
						node = ThisType::getPrevNode(node, nullptr);
				} else {
					if (node == tree->_cachedMaxNode)
						throw std::logic_error("Dereasing the begin iterator");

					if (!node)
						node = (Node *)tree->_cachedMinNode;
					else
						node = ThisType::getNextNode(node, nullptr);
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE Iterator prev() {
				Iterator iterator = *this;

				return --iterator;
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
				return node->treeKey;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->treeKey;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->treeKey;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->treeKey;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator((Node *)_getMinNode(_root), this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator beginReversed() {
			return Iterator((Node *)_cachedMaxNode, this, IteratorDirection::Reversed);
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
				constructAt<ConstIterator>(&dest, *this);
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

			PEFF_FORCEINLINE ConstIterator next() {
				ConstIterator iterator = *this;

				return ++iterator;
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

			PEFF_FORCEINLINE ConstIterator prev() {
				ConstIterator iterator = *this;

				return --iterator;
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

		PEFF_FORCEINLINE void remove(const Iterator &iterator) {
			assert(("Cannot remove the end iterator", iterator.node));
			remove(iterator.node);
		}
	};

	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false>
	using RBTree = RBTreeImpl<T, Comparator, false, IsThreeway>;
	template <typename T, typename Comparator = peff::FallibleLt<T>, bool IsThreeway = false>
	using FallibleRBTree = RBTreeImpl<T, Comparator, true, IsThreeway>;
}

#endif
