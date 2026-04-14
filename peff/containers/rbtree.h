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
		NodeBase *_cached_min_node = nullptr, *_cached_max_node = nullptr;
		size_t _num_nodes = 0;

		PEFF_CONTAINERS_API static NodeBase *_get_min_node(NodeBase *node) noexcept;
		PEFF_CONTAINERS_API static NodeBase *_get_max_node(NodeBase *node) noexcept;

		PEFF_FORCEINLINE static bool _is_red(NodeBase *node) noexcept { return node && node->color == RBColor::Red; }
		PEFF_FORCEINLINE static bool _is_black(NodeBase *node) noexcept { return (!node) || node->color == RBColor::Black; }

		PEFF_CONTAINERS_API void _l_rot(NodeBase *x) noexcept;
		PEFF_CONTAINERS_API void _r_rot(NodeBase *x) noexcept;

		PEFF_CONTAINERS_API void _insert_fix_up(NodeBase *node) noexcept;

		PEFF_CONTAINERS_API NodeBase *_remove_fix_up(NodeBase *node) noexcept;

		PEFF_CONTAINERS_API void _verify(NodeBase *node, const size_t num_black, size_t black_count) const noexcept;
		PEFF_CONTAINERS_API void _verify() const noexcept;

		PEFF_CONTAINERS_API static NodeBase *_get_next_node(const NodeBase *node, const NodeBase *last_node) noexcept;
		PEFF_CONTAINERS_API static NodeBase *_get_prev_node(const NodeBase *node, const NodeBase *first_node) noexcept;

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
		static_assert(std::is_move_constructible_v<T>, "The key must be move-constructible");
		struct Node : public RBTreeBase::NodeBase {
			T rb_value;

			PEFF_FORCEINLINE Node(T &&key) : rb_value(std::move(key)) {}
			PEFF_FORCEINLINE ~Node() {}
		};

		using NodeType = Node;
		using ComparatorType = Comparator;

	protected:
		using NodeQueryResultType = typename std::conditional<Fallible, Option<Node *>, Node *>::type;

		using ThisType = RBTreeImpl<T, Comparator, Fallible, IsThreeway>;

		Comparator _comparator;
		RcObjectPtr<Alloc> _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_alloc_single_node(T &&value) {
			Node *node = (Node *)alloc_and_construct<Node>(_allocator.get(), alignof(Node), std::move(value));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _delete_single_node(Node *node) {
			destroy_and_release<Node>(_allocator.get(), node, alignof(Node));
		}

		PEFF_FORCEINLINE void _delete_node_tree(Node *node) {
			Node *cur_node = (Node *)_get_min_node(node);
			Node *parent = (Node *)node->p;
			bool walked_root_node = false;

			while (cur_node != parent) {
				if (cur_node->r) {
					cur_node = (Node *)_get_min_node(cur_node->r);
				} else {
					Node *node_to_delete = cur_node;

					while (cur_node->p && (cur_node == cur_node->p->r)) {
						node_to_delete = cur_node;
						cur_node = (Node *)cur_node->p;
						_delete_single_node(node_to_delete);
					}

					node_to_delete = cur_node;
					cur_node = (Node *)cur_node->p;
					_delete_single_node(node_to_delete);
				}
			}
		}

		PEFF_FORCEINLINE NodeQueryResultType _get(const T &key) const {
			Node *i = (Node *)_root;

			if constexpr (Fallible) {
				while (i) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(i->rb_value, key);

						if (result.value() > 0)
							i = (Node *)i->r;
						else if (result.value() < 0)
							i = (Node *)i->l;
						else
							return i;
					} else {
						Option<bool> result;

						if ((result = _comparator(i->rb_value, key)).has_value()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(key, i->rb_value)).has_value()) {
									assert(!result.value());
									i = (Node *)i->r;
								} else {
									return NULL_OPTION;
								}
#else
								i = (Node *)i->r;
#endif
							} else if ((result = _comparator(key, i->rb_value)).has_value()) {
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
						auto &&result = _comparator(i->rb_value, key);
						if (result > 0)
							i = (Node *)i->r;
						else if (result < 0)
							i = (Node *)i->l;
						else
							return i;
					} else {
						if (_comparator(i->rb_value, key)) {
							assert(!_comparator(key, i->rb_value));
							i = (Node *)i->r;
						} else if (_comparator(key, i->rb_value)) {
							i = (Node *)i->l;
						} else
							return i;
					}
				}
			}
			return nullptr;
		}

		PEFF_FORCEINLINE NodeBase **_get_slot(const T &key, NodeBase *&parent_out) {
			NodeBase **i = (NodeBase **)&_root;
			parent_out = nullptr;

			if constexpr (Fallible) {
				while (*i) {
					parent_out = *i;

					if constexpr (IsThreeway) {
						auto &&result = _comparator(static_cast<Node *>(*i)->rb_value, key);

						if (result.value() > 0)
							i = (NodeBase **)&static_cast<Node *>(*i)->r;
						else if (result.value() < 0)
							i = (NodeBase **)&static_cast<Node *>(*i)->l;
						else
							return i;
					} else {
						Option<bool> result;

						if ((result = _comparator(static_cast<Node *>(*i)->rb_value, key)).has_value()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(key, static_cast<Node *>(*i)->rb_value)).has_value()) {
									assert(!result.value());
									i = (NodeBase **)&static_cast<Node *>(*i)->r;
								} else {
									return nullptr;
								}
#else
								i = (NodeBase **)&static_cast<Node *>(*i)->r;
#endif
							} else if ((result = _comparator(key, static_cast<Node *>(*i)->rb_value)).has_value()) {
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
						parent_out = *i;

						auto &&result = _comparator(static_cast<Node *>(*i)->rb_value, key);

						if (result > 0) {
							i = (NodeBase **)&((*i)->r);
						} else if (result < 0) {
							i = (NodeBase **)&((*i)->l);
						} else
							return nullptr;
					}
				} else {
					while (*i) {
						parent_out = *i;

						if (_comparator(static_cast<Node *>(*i)->rb_value, key)) {
							assert(!_comparator(key, static_cast<Node *>(*i)->rb_value));
							i = (NodeBase **)&((*i)->r);
						} else if (_comparator(key, static_cast<Node *>(*i)->rb_value)) {
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
				goto update_node_caches;
			}

			if constexpr (Fallible) {
				if constexpr (IsThreeway) {
					if (auto &&result = _comparator(node->rb_value, parent->rb_value); result.has_value()) {
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
					if ((result = _comparator(node->rb_value, parent->rb_value)).has_value()) {
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
					auto &&result = _comparator(node->rb_value, parent->rb_value);
					if (result > 0)
						parent->l = node;
					else if (result < 0)
						parent->r = node;
					else
						assert(false);
				} else {
					if (_comparator(node->rb_value, parent->rb_value))
						parent->l = node;
					else
						parent->r = node;
				}
			}
			node->p = parent;
			node->color = RBColor::Red;

			_insert_fix_up(node);

		update_node_caches:
			_cached_min_node = _get_min_node(_root);
			_cached_max_node = _get_max_node(_root);

			++_num_nodes;

			return true;
		}

		PEFF_FORCEINLINE Node *_remove(Node *node) {
			Node *y = (Node *)_remove_fix_up(node);

			_cached_min_node = _get_min_node(_root);
			_cached_max_node = _get_max_node(_root);

			--_num_nodes;

			return y;
		}

	public:
		PEFF_FORCEINLINE RBTreeImpl(Alloc *allocator, Comparator &&comparator) : _allocator(allocator), _comparator(std::move(comparator)) {}

		PEFF_FORCEINLINE RBTreeImpl(ThisType &&other)
			: _comparator(std::move(other._comparator)),
			  _allocator(std::move(other._allocator)) {
			_root = other._root;
			_cached_min_node = other._cached_min_node;
			_cached_max_node = other._cached_max_node;
			_num_nodes = other._num_nodes;

			other._root = nullptr;
			other._cached_min_node = nullptr;
			other._cached_max_node = nullptr;
			other._num_nodes = 0;
		}

		PEFF_FORCEINLINE ~RBTreeImpl() {
			if (_root)
				_delete_node_tree((Node *)_root);
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) noexcept {
			verify_allocator(other._allocator.get(), _allocator.get());

			clear();

			_root = other._root;
			_cached_min_node = other._cached_min_node;
			_cached_max_node = other._cached_max_node;
			_num_nodes = other._num_nodes;
			_comparator = std::move(other._comparator);
			_allocator = other._allocator;

			other._root = nullptr;
			other._cached_min_node = nullptr;
			other._cached_max_node = nullptr;
			other._num_nodes = 0;
			other._allocator = nullptr;

			return *this;
		}

		PEFF_FORCEINLINE NodeQueryResultType get_max_lteq(const T &data) {
			Node *cur_node = (Node *)_root, *max_node = NULL;

			if constexpr (Fallible) {
				while (cur_node) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(cur_node->rb_value, data);

						if (result.value() > 0) {
							max_node = cur_node;
							cur_node = (Node *)cur_node->r;
						} else if (result.value() < 0)
							cur_node = (Node *)cur_node->l;
						else
							return cur_node;
					} else {
						Option<bool> result;

						if ((result = _comparator(cur_node->rb_value, data)).has_value()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(data, cur_node->rb_value)).has_value()) {
									assert(!result.value());
									cur_node = (Node *)cur_node->r;
								} else {
									return NULL_OPTION;
								}
#else
								max_node = cur_node;
								cur_node = (Node *)cur_node->r;
#endif
							} else if ((result = _comparator(data, cur_node->rb_value)).has_value()) {
								if (result.value()) {
									cur_node = (Node *)cur_node->l;
								} else
									return cur_node;
							} else {
								return NULL_OPTION;
							}
						} else {
							return NULL_OPTION;
						}
					}
				}
			} else {
				while (cur_node) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(cur_node->rb_value, data);
						if (result > 0) {
							max_node = cur_node;
							cur_node = (Node *)cur_node->r;
						} else if (result < 0)
							cur_node = (Node *)cur_node->l;
						else
							return cur_node;
					} else {
						if (_comparator(cur_node->rb_value, data)) {
							assert(!_comparator(data, cur_node->rb_value));
							max_node = cur_node;
							cur_node = (Node *)cur_node->r;
						} else if (_comparator(data, cur_node->rb_value)) {
							cur_node = (Node *)cur_node->l;
						} else
							return cur_node;
					}
				}
			}

			return max_node;
		}

		PEFF_FORCEINLINE NodeQueryResultType get(const T &key) const {
			return _get(key);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return Whether the node is inserted successfully, false if node with the same key presents.
		[[nodiscard]] PEFF_FORCEINLINE bool insert(Node *node) {
			NodeBase *parent = nullptr;
			NodeBase **slot = _get_slot(node->rb_value, parent);

			assert (slot);

			return _insert(static_cast<Node *>(parent), node);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insert(T &&key) {
			NodeBase *parent = nullptr, **slot = _get_slot(key, parent);

			if (!slot) {
				Node *node = static_cast<Node *>(parent);
				move_assign_or_move_construct<T>(node->rb_value, std::move(key));
				return node;
			}

			Node *node = _alloc_single_node(std::move(key));
			if (!node)
				return nullptr;
			if (!insert(node)) {
				_delete_single_node(node);
				return nullptr;
			}

			return node;
		}

		PEFF_FORCEINLINE void remove(Node *node, bool delete_node = true) {
			Node *y = _remove(node);
			if (delete_node)
				_delete_single_node((Node *)y);
			else {
				y->l = nullptr;
				y->r = nullptr;
				y->p = nullptr;
			}
		}

		PEFF_FORCEINLINE void clear() {
			if (_root) {
				_delete_node_tree((Node *)_root);
				_root = nullptr;
				_num_nodes = 0;
			}
			_cached_max_node = nullptr;
			_cached_min_node = nullptr;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _num_nodes;
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return const_cast<ThisType *>(this)->_allocator.get();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) {
			verify_replaceable(_allocator.get(), rhs);

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

		static Node *get_next_node(const Node *node, const Node *last_node) noexcept {
			return (Node *)_get_next_node((const NodeBase *)node, (const NodeBase *)last_node);
		}

		static Node *get_prev_node(const Node *node, const Node *first_node) noexcept {
			return (Node *)_get_prev_node((const NodeBase *)node, (const NodeBase *)first_node);
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
				peff::construct_at<Iterator>(this, std::move(rhs));
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
					node = ThisType::get_next_node(node, nullptr);
				} else {
					node = ThisType::get_prev_node(node, nullptr);
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
					if (node == tree->_cached_min_node)
						throw std::logic_error("Dereasing the begin iterator");

					if (!node)
						node = (Node *)tree->_cached_max_node;
					else
						node = ThisType::get_prev_node(node, nullptr);
				} else {
					if (node == tree->_cached_max_node)
						throw std::logic_error("Dereasing the begin iterator");

					if (!node)
						node = (Node *)tree->_cached_min_node;
					else
						node = ThisType::get_next_node(node, nullptr);
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
				return node->rb_value;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->rb_value;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->rb_value;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->rb_value;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator((Node *)_get_min_node(_root), this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator begin_reversed() {
			return Iterator((Node *)_cached_max_node, this, IteratorDirection::Reversed);
		}
		PEFF_FORCEINLINE Iterator end_reversed() {
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
				peff::construct_at<ConstIterator>(&dest, *this);
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

		PEFF_FORCEINLINE ConstIterator begin_const() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator end_const() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator begin_const_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin_reversed());
		}
		PEFF_FORCEINLINE ConstIterator end_const_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end_reversed());
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
