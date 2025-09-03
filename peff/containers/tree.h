#ifndef _PEFF_CONTAINERS_TREE_H_
#define _PEFF_CONTAINERS_TREE_H_

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
#include <peff/utils/comparator.h>
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
		typename Comparator,
		bool Fallible>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> &&std::strict_weak_order<Comparator, T, T>)
	class RBTreeImpl : protected RBTreeBase {
	public:
		struct Node : public RBTreeBase::AbstractNode {
			T value;

			inline Node(T &&value) : value(std::move(value)) {}
			virtual ~Node() {}
		};

		using NodeType = Node;
		using ComparatorType = Comparator;

	private:
		template <bool Fallible>
		struct NodeQueryResultTypeUtil {
			using type = Node *;
		};

		template <>
		struct NodeQueryResultTypeUtil<true> {
			using type = std::optional<Node *>;
		};

	protected:
		using NodeQueryResultType = typename NodeQueryResultTypeUtil<Fallible>::type;

		using ThisType = RBTreeImpl<T, Comparator, Fallible>;

		Comparator _comparator;
		RcObjectPtr<Alloc> _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode(T &&value) {
			Node *node = (Node *)_allocator->alloc(sizeof(Node), alignof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() noexcept {
					_allocator->release(node, sizeof(Node), alignof(Node));
				});
			constructAt<Node>(node, std::move(value));
			scopeGuard.release();

			return node;
		}

		PEFF_FORCEINLINE void _deleteSingleNode(Node *node) {
			std::destroy_at<Node>(node);
			_allocator->release(node, sizeof(Node), alignof(Node));
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

		PEFF_FORCEINLINE typename NodeQueryResultType _get(const T &key) const {
			Node *i = (Node *)_root;

			if constexpr (Fallible) {
				std::optional<bool> result;

				while (i) {
					if ((result = _comparator(i->value, key)).has_value()) {
						if (result.value()) {
#ifndef _NDEBUG
							if ((result = _comparator(key, i->value)).has_value()) {
								assert(!result.value());
								i = (Node *)i->r;
							} else {
								return std::nullopt;
							}
#else
							i = (Node *)i->r;
#endif
						} else if ((result = _comparator(key, i->value)).has_value()) {
							if (result.value()) {
								i = (Node *)i->l;
							} else
								return i;
						} else {
							return std::nullopt;
						}
					} else {
						return std::nullopt;
					}
				}
			} else {
				while (i) {
					if (_comparator(i->value, key)) {
						assert(!_comparator(key, i->value));
						i = (Node *)i->r;
					} else if (_comparator(key, i->value)) {
						i = (Node *)i->l;
					} else
						return i;
				}
			}
			return nullptr;
		}

		PEFF_FORCEINLINE Node **_getSlot(const T &key, Node *&parentOut) {
			Node **i = (Node **)&_root;
			parentOut = nullptr;

			if constexpr (Fallible) {
				std::optional<bool> result;

				while (*i) {
					parentOut = *i;

					if ((result = _comparator((*i)->value, key)).has_value()) {
						if (result.value()) {
#ifndef _NDEBUG
							if ((result = _comparator(key, (*i)->value)).has_value()) {
								assert(!result.value());
								i = (Node **)&(*i)->r;
							} else {
								return nullptr;
							}
#else
							i = (Node **)&(*i)->r;
#endif
						} else if ((result = _comparator(key, (*i)->value)).has_value()) {
							if (result.value()) {
								i = (Node **)&(*i)->l;
							} else
								return i;
						} else {
							return nullptr;
						}
					} else {
						return nullptr;
					}
				}
			} else {
				while (*i) {
					parentOut = *i;

					if (_comparator((*i)->value, key)) {
						assert(!_comparator(key, (*i)->value));
						i = (Node **)&((*i)->r);
					} else if (_comparator(key, (*i)->value)) {
						i = (Node **)&((*i)->l);
					} else
						return nullptr;
				}
			}
			return i;
		}

		PEFF_FORCEINLINE bool _insert(Node **slot, Node *parent, Node *node) {
			assert(!node->l);
			assert(!node->r);

			if (!_root) {
				_root = node;
				node->color = RBColor::Black;
				goto updateNodeCaches;
			}

			if constexpr (Fallible) {
				{
					std::optional<bool> result;
					if ((result = _comparator(node->value, parent->value)).has_value()) {
						if (result.value())
							parent->l = node;
						else
							parent->r = node;
					} else {
						return false;
					}
					node->p = parent;
					node->color = RBColor::Red;

					_insertFixUp(node);
				}
			} else {
				{
					if (_comparator(node->value, parent->value))
						parent->l = node;
					else
						parent->r = node;
					node->p = parent;
					node->color = RBColor::Red;

					_insertFixUp(node);
				}
			}

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

		virtual inline ~RBTreeImpl() {
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

		PEFF_FORCEINLINE Node *getMaxLteqNode(const T &data) {
			Node *curNode = (Node *)_root, *maxNode = NULL;

			if constexpr (Fallible) {
				std::optional<bool> result;

				while (curNode) {
					if ((result = _comparator(curNode->value, data)).has_value()) {
						if (result.value()) {
							if ((result = _comparator(data, curNode->value)).has_value()) {
								assert(!result.value());
								maxNode = curNode;
								curNode = (Node *)curNode->r;
							} else {
								return nullptr;
							}
						} else if ((result = _comparator(data, curNode->value)).has_value()) {
							if (result.value()) {
								curNode = (Node *)curNode->l;
							} else {
								return curNode;
							}
						} else {
							return nullptr;
						}
					} else {
						return nullptr;
					}
				}
			} else {
				while (curNode) {
					if (_comparator(curNode->value, data)) {
						assert(!_comparator(data, curNode->value));
						maxNode = curNode;
						curNode = (Node *)curNode->r;
					} else if (_comparator(data, curNode->value)) {
						curNode = (Node *)curNode->l;
					} else
						return curNode;
				}
			}

			return maxNode;
		}

		PEFF_FORCEINLINE typename NodeQueryResultType get(const T &key) const {
			return _get(key);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return Whether the node is inserted successfully, false if node with the same key presents.
		[[nodiscard]] PEFF_FORCEINLINE bool insert(Node *node) {
			Node *parent = nullptr, **slot = _getSlot(node->value, parent);

			if (!slot)
				return false;

			return _insert(slot, parent, node);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insert(T &&key) {
			Node *parent = nullptr, **slot = _getSlot(key, parent);

			if (!slot)
				return parent;

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

		PEFF_FORCEINLINE size_t size() {
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

	template <typename T, typename Comparator = std::less<T>>
	using RBTree = RBTreeImpl<T, Comparator, false>;
	template <typename T, typename Comparator = peff::FallibleLt<T>>
	using FallibleRBTree = RBTreeImpl<T, Comparator, true>;
}

#endif
