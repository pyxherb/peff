#ifndef _PEFF_CONTAINERS_RBTREE_H_
#define _PEFF_CONTAINERS_RBTREE_H_

#include <cassert>
#include <type_traits>

#if PEFF_CPLUSPLUS >= 202002L
	#include <concepts>
#endif

#include "basedefs.h"
#include "misc.h"
#include <peff/base/alloc.h>
#include <peff/base/misc.h>
#include <peff/base/scope_guard.h>
#include <peff/utils/fallible_cmp.h>
#include <peff/utils/misc.h>
#include <peff/utils/option.h>
#include <peff/utils/result.h>

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

		PEFF_CONTAINERS_API static NodeBase *_get_min_node(PEFF_NULLABLE NodeBase *node) noexcept;
		PEFF_CONTAINERS_API static NodeBase *_get_max_node(PEFF_NULLABLE NodeBase *node) noexcept;

		PEFF_FORCEINLINE static bool _is_red(PEFF_NULLABLE NodeBase *node) noexcept { return node && node->color == RBColor::Red; }
		PEFF_FORCEINLINE static bool _is_black(PEFF_NULLABLE NodeBase *node) noexcept { return (!node) || node->color == RBColor::Black; }

		PEFF_CONTAINERS_API void _l_rot(PEFF_NONNULL NodeBase *x) noexcept;
		PEFF_CONTAINERS_API void _r_rot(PEFF_NONNULL NodeBase *x) noexcept;

		PEFF_CONTAINERS_API void _insert_fix_up(PEFF_NONNULL NodeBase *node) noexcept;

		PEFF_CONTAINERS_API PEFF_NONNULL_RET NodeBase *_remove_fix_up(PEFF_NONNULL NodeBase *node) noexcept;

		PEFF_CONTAINERS_API void _verify(PEFF_NULLABLE NodeBase *node, const size_t num_black, size_t black_count) const noexcept;
		PEFF_CONTAINERS_API void _verify() const noexcept;

		PEFF_CONTAINERS_API PEFF_NULLABLE_RET static NodeBase *_get_next_node(PEFF_NULLABLE const NodeBase *node, PEFF_NULLABLE const NodeBase *last_node) noexcept;
		PEFF_CONTAINERS_API PEFF_NULLABLE_RET static NodeBase *_get_prev_node(PEFF_NULLABLE const NodeBase *node, PEFF_NULLABLE const NodeBase *first_node) noexcept;

		PEFF_CONTAINERS_API RBTreeBase() noexcept;
		PEFF_CONTAINERS_API ~RBTreeBase();
	};

	/// @brief A red-black tree implementation.
	/// @tparam T The type of the stored elements.
	/// @tparam Comparator The type of the comparator used to order the elements.
	/// @tparam Fallible Whether the comparator is allowed to fail.
	/// @tparam IsThreeway Whether the comparator yields a three-way ordering. Combined with Fallible,
	///         this means the comparator yields a peff::PartialOrd, i.e. the comparison itself may
	///         fail with peff::PARTIAL_UNORDERED. In that case every operation that needs to compare
	///         keys can fail and reports it through its return value, leaving the tree unmodified.
	template <typename T,
		typename Comparator,
		bool Fallible,
		bool IsThreeway>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> || std::invocable<Comparator, T &&, T &&>)
	class RBTreeImpl final : protected RBTreeBase {
	public:
		static_assert(std::is_move_constructible_v<T>, "The key must be move-constructible");
		static_assert(!(Fallible && IsThreeway) || std::is_invocable_r_v<PartialOrd, Comparator, const T &, const T &>,
			"A fallible three-way comparator must return peff::PartialOrd");
		static_assert(!std::is_invocable_r_v<PartialOrd, Comparator, const T &, const T &> || (Fallible && IsThreeway),
			"A comparator returning peff::PartialOrd requires a fallible three-way tree, that is, both "
			"Fallible and IsThreeway must be enabled");

		struct Node : public RBTreeBase::NodeBase {
			T rb_value;

			PEFF_FORCEINLINE Node(T &&key) : rb_value(std::move(key)) {}
			PEFF_FORCEINLINE ~Node() {}
		};

		using NodeType = Node;
		using ComparatorType = Comparator;

	private:
		/// @brief The result type of an operation that may fail to compare two keys.
		template <typename X>
		using Outcome = std::conditional_t<Fallible, Result<X, Unordered>, X>;

		/// @brief The result type of a query that may fail to compare two keys.
		using NodeQueryResultType = std::conditional_t<Fallible, Result<Option<Node *>, Unordered>, Node *>;

		using ThisType = RBTreeImpl<T, Comparator, Fallible, IsThreeway>;

		Comparator _comparator;
		RcObjectPtr<Alloc> _allocator;

		/// @brief Perform a fallible three-way comparison through the comparator.
		/// @tparam L The type of the left-hand operand.
		/// @tparam R The type of the right-hand operand.
		/// @param lhs The left-hand operand.
		/// @param rhs The right-hand operand.
		/// @return The comparison result, or peff::PARTIAL_UNORDERED when the comparison failed,
		///         that is, neither "result > 0", "result < 0" nor "result == 0" holds.
		template <typename L, typename R>
		[[nodiscard]] PEFF_FORCEINLINE PartialOrd _compare_partial(const L &lhs, const R &rhs) const {
			const PartialOrd result = _comparator(lhs, rhs);

			if (result < 0)
				return PARTIAL_LT;
			if (result > 0)
				return PARTIAL_GT;
			if (result == 0)
				return PARTIAL_EQ;

			return PARTIAL_UNORDERED;
		}

		/// @brief Build the "not found" outcome of a query.
		[[nodiscard]] PEFF_FORCEINLINE static NodeQueryResultType _query_not_found() {
			if constexpr (Fallible)
				return Option<Node *>(NULLOPT);
			else
				return nullptr;
		}

		/// @brief Build the "found" outcome of a query.
		/// @param node The node that has been found.
		[[nodiscard]] PEFF_FORCEINLINE static NodeQueryResultType _query_found(PEFF_NONNULL Node *node) {
			if constexpr (Fallible)
				return Option<Node *>(node);
			else
				return node;
		}

		/// @brief Build the failure outcome of a query, that is, the comparison failed.
		[[nodiscard]] PEFF_FORCEINLINE static NodeQueryResultType _query_failure() {
			if constexpr (Fallible)
				return Unordered();
			else
				return nullptr;
		}

		[[nodiscard]] PEFF_FORCEINLINE PEFF_NULLABLE_RET Node *_alloc_single_node(T &&value) {
			Node *node = (Node *)alloc_and_construct<Node>(_allocator.get(), alignof(Node), std::move(value));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _delete_single_node(PEFF_NONNULL Node *node) {
			destroy_and_release<Node>(_allocator.get(), node, alignof(Node));
		}

		PEFF_FORCEINLINE void _delete_node_tree(PEFF_NONNULL Node *node) {
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

		template <typename U>
		PEFF_FORCEINLINE NodeQueryResultType _get(const U &key) const {
			Node *i = (Node *)_root;

			if constexpr (Fallible) {
				while (i) {
					if constexpr (IsThreeway) {
						const PartialOrd result = _compare_partial(i->rb_value, key);

						if (result < 0) // The stored key is less than the queried one.
							i = (Node *)i->r;
						else if (result > 0) // The stored key is greater than the queried one.
							i = (Node *)i->l;
						else if (result == 0)
							return _query_found(i);
						else // The comparison failed.
							return _query_failure();
					} else {
						Option<bool> result;

						if ((result = _comparator(i->rb_value, key)).has_value()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(key, i->rb_value)).has_value()) {
									assert(!result.value());
									i = (Node *)i->r;
								} else {
									return _query_failure();
								}
#else
								i = (Node *)i->r;
#endif
							} else if ((result = _comparator(key, i->rb_value)).has_value()) {
								if (result.value()) {
									i = (Node *)i->l;
								} else
									return _query_found(i);
							} else {
								return _query_failure();
							}
						} else {
							return _query_failure();
						}
					}
				}
			} else {
				while (i) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(i->rb_value, key);
						if (result < 0) {
							assert(_comparator(key, i->rb_value) > 0);
							i = (Node *)i->r;
						} else if (result > 0) {
							assert(_comparator(key, i->rb_value) < 0);
							i = (Node *)i->l;
						} else {
							assert(_comparator(key, i->rb_value) == 0);
							return i;
						}
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
			return _query_not_found();
		}

		/// @brief Locate the insertion slot for a key.
		/// @param key The key to look for.
		/// @param parent_out Receives the parent of the insertion slot, or the existing node when
		///        the key is already present.
		/// @param unordered_out When not null, receives true if the comparison failed. Only used when
		///        both Fallible and IsThreeway are enabled.
		/// @return The address of the child pointer that should receive the new node, or null when
		///         the key is already present or the comparison failed.
		PEFF_FORCEINLINE NodeBase **_get_slot(const T &key, NodeBase *&parent_out, PEFF_NULLABLE bool *unordered_out = nullptr) {
			NodeBase **i = (NodeBase **)&_root;
			parent_out = nullptr;

			if constexpr (Fallible) {
				while (*i) {
					parent_out = *i;

					if constexpr (IsThreeway) {
						const PartialOrd result = _compare_partial(static_cast<Node *>(*i)->rb_value, key);

						if (result < 0) // The stored key is less than the queried one.
							i = (NodeBase **)&((*i)->r);
						else if (result > 0) // The stored key is greater than the queried one.
							i = (NodeBase **)&((*i)->l);
						else if (result == 0)
							return nullptr;
						else {
							// The comparison failed.
							if (unordered_out)
								*unordered_out = true;
							return nullptr;
						}
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

						if (result < 0) {
							assert(_comparator(key, static_cast<Node *>(*i)->rb_value) > 0);
							i = (NodeBase **)&((*i)->r);
						} else if (result > 0) {
							assert(_comparator(key, static_cast<Node *>(*i)->rb_value) < 0);
							i = (NodeBase **)&((*i)->l);
						} else {
							assert(_comparator(key, static_cast<Node *>(*i)->rb_value) == 0);
							return nullptr;
						}
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

		template <typename U>
		PEFF_FORCEINLINE NodeQueryResultType _get_max_lteq(const U &data) {
			Node *cur_node = (Node *)_root, *max_node = NULL;

			if constexpr (Fallible) {
				while (cur_node) {
					if constexpr (IsThreeway) {
						const PartialOrd result = _compare_partial(cur_node->rb_value, data);

						if (result < 0) { // The stored key is less than the queried one.
							max_node = cur_node;
							cur_node = (Node *)cur_node->r;
						} else if (result > 0) // The stored key is greater than the queried one.
							cur_node = (Node *)cur_node->l;
						else if (result == 0)
							return _query_found(cur_node);
						else // The comparison failed.
							return _query_failure();
					} else {
						Option<bool> result;

						if ((result = _comparator(cur_node->rb_value, data)).has_value()) {
							if (result.value()) {
#ifndef NDEBUG
								if ((result = _comparator(data, cur_node->rb_value)).has_value()) {
									assert(!result.value());
									cur_node = (Node *)cur_node->r;
								} else {
									return _query_failure();
								}
#endif
								max_node = cur_node;
								cur_node = (Node *)cur_node->r;
							} else if ((result = _comparator(data, cur_node->rb_value)).has_value()) {
								if (result.value()) {
									cur_node = (Node *)cur_node->l;
								} else
									return _query_found(cur_node);
							} else {
								return _query_failure();
							}
						} else {
							return _query_failure();
						}
					}
				}
			} else {
				while (cur_node) {
					if constexpr (IsThreeway) {
						auto &&result = _comparator(cur_node->rb_value, data);
						if (result < 0) {
							assert(_comparator(data, cur_node->rb_value) > 0);
							max_node = cur_node;
							cur_node = (Node *)cur_node->r;
						} else if (result > 0) {
							assert(_comparator(data, cur_node->rb_value) < 0);
							cur_node = (Node *)cur_node->l;
						} else {
							assert(_comparator(data, cur_node->rb_value) == 0);
							return cur_node;
						}
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

			return max_node ? _query_found(max_node) : _query_not_found();
		}

		PEFF_FORCEINLINE bool _insert(PEFF_NONNULL Node *parent, PEFF_NONNULL Node *node) {
			assert(!node->l);
			assert(!node->r);

			if (!_root) {
				_root = node;
				node->color = RBColor::Black;
				goto update_node_caches;
			}

			if constexpr (Fallible) {
				if constexpr (IsThreeway) {
					const PartialOrd result = _compare_partial(node->rb_value, parent->rb_value);

					if (result < 0) // The new key is less than the parent key.
						parent->l = node;
					else if (result > 0) // The new key is greater than the parent key.
						parent->r = node;
					else {
						// Either the comparison failed (peff::PARTIAL_UNORDERED) or the caller
						// provided a duplicate key (peff::PARTIAL_EQ). Nothing has been linked
						// to the tree so far, so the tree stays untouched.
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
				if constexpr (IsThreeway) {
					auto result = _comparator(node->rb_value, parent->rb_value);
					if (result < 0) {
						assert(_comparator(parent->rb_value, node->rb_value) > 0);
						parent->l = node;
					} else if (result > 0) {
						assert(_comparator(parent->rb_value, node->rb_value) < 0);
						parent->r = node;
					} else {
						assert(_comparator(parent->rb_value, node->rb_value) == 0);
						assert(false);
					}
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

		PEFF_FORCEINLINE Node *_remove(PEFF_NONNULL Node *node) {
			Node *y = (Node *)_remove_fix_up(node);

			_cached_min_node = _get_min_node(_root);
			_cached_max_node = _get_max_node(_root);

			--_num_nodes;

			return y;
		}

	public:
		PEFF_FORCEINLINE RBTreeImpl(PEFF_NONNULL Alloc *allocator, Comparator &&comparator) : _allocator(allocator), _comparator(std::move(comparator)) {}

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
			return _get_max_lteq<T>(data);
		}

		template <typename U>
		PEFF_FORCEINLINE NodeQueryResultType get_max_lteq_alt(const U &data) {
			return _get_max_lteq<U>(data);
		}

		PEFF_FORCEINLINE NodeQueryResultType get(const T &key) const {
			return _get<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE NodeQueryResultType get_alt(const U &key) const {
			if constexpr (Fallible && IsThreeway) {
				static_assert(std::is_invocable_r_v<PartialOrd, Comparator, const T &, const U &>,
					"The query type is not comparable with the key in the fallible three-way mode");
			} else if constexpr (IsThreeway) {
				static_assert(std::is_invocable_v<Comparator, const T &, const U &>, "The query type is not comparable with the key");
			} else {
				static_assert(
					(std::is_invocable_v<Comparator, const T &, const U &> &&
						std::is_invocable_v<Comparator, const U &, const T &>) ||
						(std::is_convertible_v<U, T>),
					"The query type is not comparable with the key");
			}
			return _get<U>(key);
		}

		/// @brief Insert a node into the tree.
		/// @param node Node to be inserted.
		/// @return The pointer to the stored key on success. On failure it is either a null pointer
		///         (out of memory, or the key is already present) or, in the fallible mode, an
		///         Unordered error when the comparison failed. In the latter two cases the tree is
		///         left unmodified and the caller keeps the ownership of the node.
		[[nodiscard]] PEFF_FORCEINLINE Outcome<T *> insert(PEFF_NONNULL Node *node) {
			NodeBase *parent = nullptr;
			bool unordered = false;
			NodeBase **slot = _get_slot(node->rb_value, parent, &unordered);

			if constexpr (Fallible) {
				if (unordered) // The comparison failed.
					return Unordered();
			}

			assert(slot);

			if (_insert(static_cast<Node *>(parent), node))
				return &node->rb_value;
			return nullptr;
		}

		/// @brief Insert a key into the tree.
		/// @param key Key to be inserted.
		/// @return The inserted (or already present) node on success, a null pointer when out of
		///         memory, or, in the fallible mode, an Unordered error when the comparison failed.
		[[nodiscard]] PEFF_FORCEINLINE Outcome<Node *> insert(T &&key) {
			NodeBase *parent = nullptr;
			bool unordered = false;
			NodeBase **slot = _get_slot(key, parent, &unordered);

			if constexpr (Fallible) {
				if (unordered) // The comparison failed, the tree is left unmodified.
					return Unordered();
			}

			if (!slot) {
				Node *node = static_cast<Node *>(parent);
				move_assign_or_move_construct<T>(node->rb_value, std::move(key));
				return node;
			}

			Node *node = _alloc_single_node(std::move(key));
			if (!node)
				return nullptr;

			if constexpr (Fallible) {
				Outcome<T *> inserted = insert(node);

				if (inserted.is_error()) {
					// The comparison failed while linking the node: roll it back.
					_delete_single_node(node);
					return Unordered();
				}

				if (!inserted.value()) {
					// The key turned out to be duplicated: roll the freshly allocated node back.
					_delete_single_node(node);
					return nullptr;
				}
			} else {
				if (!insert(node)) {
					// The key turned out to be duplicated: roll the freshly allocated node back.
					_delete_single_node(node);
					return nullptr;
				}
			}

			return node;
		}

		PEFF_FORCEINLINE peff::Option<T> remove(PEFF_NONNULL Node *node, bool delete_node = true) {
			Node *y = _remove(node);
			if (delete_node) {
				T data = std::move(y->rb_value);
				_delete_single_node((Node *)y);
				return data;
			}
			y->l = nullptr;
			y->r = nullptr;
			y->p = nullptr;
			return peff::NULLOPT;
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

		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return const_cast<ThisType *>(this)->_allocator.get();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) {
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

		PEFF_NULLABLE_RET static Node *get_next_node(PEFF_NULLABLE const Node *node, PEFF_NULLABLE const Node *last_node) noexcept {
			return (Node *)_get_next_node((const NodeBase *)node, (const NodeBase *)last_node);
		}

		PEFF_NULLABLE_RET static Node *get_prev_node(PEFF_NULLABLE const Node *node, PEFF_NULLABLE const Node *first_node) noexcept {
			return (Node *)_get_prev_node((const NodeBase *)node, (const NodeBase *)first_node);
		}

		struct Iterator {
			Node *node;
			ThisType *tree;
			IteratorDirection direction;

			PEFF_FORCEINLINE Iterator(
				PEFF_NULLABLE Node *node,
				PEFF_NONNULL ThisType *tree,
				IteratorDirection direction)
				: node(node),
				  tree(tree),
				  direction(direction) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;

			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				PEFF_ASSERT(direction == rhs.direction, "Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				return *this;
			}
			PEFF_FORCEINLINE Iterator &operator=(Iterator &&rhs) noexcept {
				PEFF_ASSERT(direction == rhs.direction, "Incompatible iterator direction");
				peff::construct_at<Iterator>(this, std::move(rhs));
				return *this;
			}

			PEFF_FORCEINLINE bool copy(Iterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				PEFF_ASSERT(node, "Increasing the end iterator");

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

			PEFF_FORCEINLINE Iterator next() const {
				Iterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					PEFF_ASSERT(node != tree->_cached_min_node, "Dereasing the begin iterator");

					if (!node)
						node = (Node *)tree->_cached_max_node;
					else
						node = ThisType::get_prev_node(node, nullptr);
				} else {
					PEFF_ASSERT(node != tree->_cached_max_node, "Dereasing the begin iterator");

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

			PEFF_FORCEINLINE Iterator prev() const {
				Iterator iterator = *this;

				return --iterator;
			}

			PEFF_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				PEFF_ASSERT(tree == it.tree, "Cannot compare iterators from different trees");
				return node == it.node;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &&rhs) const {
				const Iterator it = rhs;
				return *this == it;
			}

			PEFF_FORCEINLINE bool operator!=(PEFF_IN const Node *node) const noexcept {
				return node != node;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &it) const {
				PEFF_ASSERT(tree == it.tree, "Cannot compare iterators from different trees");
				return node != it.node;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE T &operator*() {
				PEFF_ASSERT(node, "Deferencing the end iterator");
				return node->rb_value;
			}

			PEFF_FORCEINLINE T &operator*() const {
				PEFF_ASSERT(node, "Deferencing the end iterator");
				return node->rb_value;
			}

			PEFF_FORCEINLINE T *operator->() {
				PEFF_ASSERT(node, "Deferencing the end iterator");
				return &node->rb_value;
			}

			PEFF_FORCEINLINE T *operator->() const {
				PEFF_ASSERT(node, "Deferencing the end iterator");
				return &node->rb_value;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator((Node *)_get_min_node(_root), this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator rbegin() {
			return Iterator((Node *)_cached_max_node, this, IteratorDirection::Reversed);
		}
		PEFF_FORCEINLINE Iterator rend() {
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

			PEFF_FORCEINLINE ConstIterator next() const {
				ConstIterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE IteratorDirection direction() const noexcept {
				return _iterator.direction;
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

			PEFF_FORCEINLINE ConstIterator prev() const {
				ConstIterator iterator = *this;

				return --iterator;
			}

			PEFF_FORCEINLINE bool operator==(const ConstIterator &it) const {
				return _iterator == it._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &it) const {
				return _iterator != it._iterator;
			}

			PEFF_FORCEINLINE bool operator==(PEFF_IN const Node *node) const {
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

		PEFF_FORCEINLINE ConstIterator cbegin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator cend() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
		PEFF_FORCEINLINE ConstIterator crbegin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rbegin());
		}
		PEFF_FORCEINLINE ConstIterator cend_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rend());
		}

		PEFF_FORCEINLINE peff::Option<T> remove(const Iterator &iterator) {
			PEFF_ASSERT(iterator.node, "Cannot remove the end iterator");
			return remove(iterator.node);
		}
	};

	/// @brief A red-black tree with a strict comparator.
	/// @tparam T The type of the stored elements.
	/// @tparam Comparator The type of the comparator used to order the elements.
	/// @tparam IsThreeway Whether the comparator yields a three-way (but never failing) ordering.
	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false>
	using RBTree = RBTreeImpl<T, Comparator, false, IsThreeway>;

	/// @brief A red-black tree with a comparator that is allowed to fail.
	/// @tparam T The type of the stored elements.
	/// @tparam Comparator The type of the comparator used to order the elements.
	/// @tparam IsThreeway Whether the comparator yields a three-way ordering. When enabled, the
	///         comparator must return peff::PartialOrd and every key comparison may fail with
	///         peff::PARTIAL_UNORDERED, so that the tree is ordered by a partial order.
	template <typename T, typename Comparator = peff::FallibleLt<T>, bool IsThreeway = false>
	using FallibleRBTree = RBTreeImpl<T, Comparator, true, IsThreeway>;
}

#endif
