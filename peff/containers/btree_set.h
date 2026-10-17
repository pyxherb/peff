#ifndef _PEFF_CONTAINERS_BTREE_SET_H_
#define _PEFF_CONTAINERS_BTREE_SET_H_

#include "basedefs.h"
#include <peff/base/misc.h>
#include <peff/utils/misc.h>
#include <peff/utils/option.h>
#include <peff/utils/result.h>
#include <peff/base/alloc.h>

namespace peff {
	struct BTreeNodeBase {
		BTreeNodeBase *parent = nullptr;
		size_t num_keys = 0;
		size_t num_children = 0;
		// Children and keys are defined in the implementation type.
	};

	template <typename T>
	constexpr size_t _calc_btree_default_node_deg() {
		const size_t num = (256 - sizeof(BTreeNodeBase)) / (sizeof(T) + sizeof(void *));
		return num / 2 < 2 ? 2 : num / 2;
	}

	/// @brief An ordered set backed by a B-tree.
	/// @tparam T The type of the stored elements.
	/// @tparam Comparator The type of the comparator used to order the elements.
	/// @tparam IsThreeway Whether the comparator yields a three-way ordering.
	/// @tparam Degree The degree of the B-tree.
	/// @tparam Fallible Whether the comparator is allowed to fail. When enabled together with
	///         IsThreeway, the comparator must return peff::PartialOrd, and every operation that
	///         compares keys may fail with peff::Unordered, which is reported through the returned
	///         peff::Result. In that case the tree is left unmodified when the comparison fails.
	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false, size_t Degree = _calc_btree_default_node_deg<T>(), bool Fallible = false>
	class BTreeSet final {
	private:
		using ThisType = BTreeSet<T, Comparator, IsThreeway, Degree, Fallible>;

		static_assert(!Fallible || IsThreeway,
			"The fallible mode of BTreeSet requires a three-way comparator that returns peff::PartialOrd");

		/// @brief Whether the comparator yields a fallible three-way ordering, i.e. a peff::PartialOrd.
		static constexpr bool _is_partial_ordered = Fallible && IsThreeway;

		/// @brief Whether an alternative-typed key can be compared with the stored keys.
		template <typename U>
		static constexpr bool _is_alt_queryable =
			std::is_invocable_v<Comparator, const T &, const U &> && std::is_invocable_v<Comparator, const U &, const T &>;

	public:
		struct Node : public BTreeNodeBase {
			char keys alignas(T)[sizeof(T) * (Degree * 2 - 1)];
			size_t num_keys = 0;
			Node *children[Degree * 2] = { nullptr };
			size_t num_children = 0;

			Node *undo_new_sibling = nullptr;
			size_t undo_index = 0;
			bool has_undo = false;

			PEFF_FORCEINLINE T &key(size_t index) noexcept {
				PEFF_ASSERT(index < num_keys, "Key access out-of-range");
				return reinterpret_cast<T *>(keys)[index];
			}
			PEFF_FORCEINLINE const T &key(size_t index) const noexcept {
				PEFF_ASSERT(index < num_keys, "Key access out-of-range");
				return reinterpret_cast<const T *>(keys)[index];
			}

			PEFF_FORCEINLINE T &push_key(T &&key) noexcept {
				PEFF_ASSERT(num_keys < std::size(keys) / sizeof(T), "Key capacity exceeded");
				peff::construct_at<T>(reinterpret_cast<T *>(keys) + num_keys, std::move(key));
				++num_keys;
				return reinterpret_cast<T *>(keys)[num_keys - 1];
			}

			PEFF_FORCEINLINE T &insert_key(size_t index, T &&key) noexcept {
				PEFF_ASSERT(num_keys < std::size(keys) / sizeof(T), "Key capacity exceeded");
				if constexpr (std::is_trivial_v<T>) {
					memmove(keys + sizeof(T) * (index + 1), keys + sizeof(T) * index, sizeof(T) * (num_keys - index));
					reinterpret_cast<T *>(keys)[index] = std::move(key);
				} else {
					T *base = reinterpret_cast<T *>(keys);
					for (size_t i = num_keys; i > index; --i) {
						peff::construct_at<T>(&base[i], std::move(base[i - 1]));
						std::destroy_at<T>(&base[i - 1]);
					}
					peff::construct_at<T>(&base[index], std::move(key));
				}
				++num_keys;
				return this->key(index);
			}

			PEFF_FORCEINLINE void remove_key(size_t index) noexcept {
				PEFF_ASSERT(num_keys > 0, "Invalid key number");
				if constexpr (std::is_trivial_v<T>) {
					memmove(keys + sizeof(T) * index, keys + sizeof(T) * (index + 1), sizeof(T) * (num_keys - index - 1));
				} else {
					T *base = reinterpret_cast<T *>(keys);
					std::destroy_at<T>(&base[index]);
					for (size_t i = index + 1; i < num_keys; ++i) {
						peff::construct_at<T>(&base[i - 1], std::move(base[i]));
						std::destroy_at<T>(&base[i]);
					}
				}
				--num_keys;
			}

			PEFF_FORCEINLINE Node *child_node(size_t index) const noexcept {
				PEFF_ASSERT(index < num_children, "Child node access out-of-range");
				return children[index];
			}
			PEFF_FORCEINLINE void push_child_node(Node *node) noexcept {
				PEFF_ASSERT(num_children < std::size(children), "Child capacity exceeded");
				children[num_children] = node;
				node->parent = this;
				++num_children;
			}
			PEFF_FORCEINLINE void insert_child_node(size_t index, Node *node) noexcept {
				PEFF_ASSERT(num_children < std::size(children), "Child capacity exceeded");
				memmove(children + (index + 1), children + index, sizeof(void *) * (num_children - index));
				children[index] = node;
				node->parent = this;
				++num_children;
			}
			PEFF_FORCEINLINE void remove_child_node(size_t index) noexcept {
				PEFF_ASSERT(num_children > 0, "Invalid child number");
				memmove(children + index, children + (index + 1), sizeof(void *) * (num_children - index - 1));
				--num_children;
			}

			PEFF_FORCEINLINE void destroy_keys() noexcept {
				for (size_t i = 0; i < num_keys; ++i)
					std::destroy_at<T>(&key(i));
			}
			PEFF_FORCEINLINE ~Node() {
				destroy_keys();
			}
			PEFF_FORCEINLINE bool is_leaf() const noexcept { return !num_children; }
		};

		using NodeType = Node;

		/// @brief The result of an operation that may fail to compare two keys. In the fallible mode
		///        it is a peff::Result whose error type is peff::Unordered, otherwise it is the plain
		///        value type of the operation.
		template <typename X>
		using Outcome = std::conditional_t<Fallible, Result<X, Unordered>, X>;

		/// @brief The result of a lookup that yields a reference to the stored element.
		using AtResult = std::conditional_t<Fallible, Result<T *, Unordered>, T &>;

		/// @brief The result of a lookup that yields an immutable reference to the stored element.
		using ConstAtResult = std::conditional_t<Fallible, Result<const T *, Unordered>, const T &>;

	private:
		Node *_root = nullptr;
		size_t _num_elements = 0;
		peff::RcObjectPtr<peff::Alloc> _allocator;
		Comparator _cmp;

		PEFF_FORCEINLINE Node *_alloc_node() const noexcept {
			return peff::alloc_and_construct<Node>(_allocator.get(), alignof(Node));
		}

		PEFF_FORCEINLINE void _dealloc_node(Node *node) const noexcept {
			peff::destroy_and_release<Node>(_allocator.get(), node, alignof(Node));
		}

		/// @brief Release every node of the tree.
		PEFF_FORCEINLINE void _destroy_tree() noexcept {
			if (!_root)
				return;

			Node *cur = _root;
			_root->undo_index = 0;

			while (cur) {
				if (cur->is_leaf()) {
					Node *parent = static_cast<Node *>(cur->parent);
					if (parent) {
						parent->undo_index++;
					}
					_dealloc_node(cur);
					cur = parent;
				} else {
					if (cur->undo_index < cur->num_children) {
						Node *child = cur->children[cur->undo_index];
						child->undo_index = 0;
						cur = child;
					} else {
						Node *parent = static_cast<Node *>(cur->parent);
						if (parent) {
							parent->undo_index++;
						}
						_dealloc_node(cur);
						cur = parent;
					}
				}
			}

			_root = nullptr;
			_num_elements = 0;
		}

		PEFF_FORCEINLINE bool _lt(const T &lhs, const T &rhs) const noexcept {
			if constexpr (IsThreeway) {
				auto result = _cmp(lhs, rhs);
				if (result < 0) {
					assert(_cmp(rhs, lhs) > 0);
					return true;
				} else if (result > 0) {
					assert(_cmp(rhs, lhs) < 0);
					return false;
				}
				assert(_cmp(rhs, lhs) == 0);
				return false;
			} else {
				return _cmp(lhs, rhs);
			}
		}

		PEFF_FORCEINLINE bool _gt(const T &lhs, const T &rhs) const noexcept {
			if constexpr (IsThreeway) {
				auto result = _cmp(lhs, rhs);
				if (result > 0) {
					assert(_cmp(rhs, lhs) < 0);
					return true;
				} else if (result < 0) {
					assert(_cmp(rhs, lhs) > 0);
					return false;
				}
				assert(_cmp(rhs, lhs) == 0);
				return false;
			} else {
				return _cmp(rhs, lhs);
			}
		}

		/// @brief Compare two keys, yielding a negative, zero or positive value.
		/// @tparam L The type of the left-hand operand.
		/// @tparam R The type of the right-hand operand.
		/// @param lhs The left-hand operand.
		/// @param rhs The right-hand operand.
		/// @return A negative value when lhs precedes rhs, zero when they are equivalent, and a
		///         positive value when lhs follows rhs.
		template <typename L, typename R>
		PEFF_FORCEINLINE int _compare(const L &lhs, const R &rhs) const noexcept {
			if constexpr (IsThreeway) {
				auto result = _cmp(lhs, rhs);
				if (result < 0) {
					assert(_cmp(rhs, lhs) > 0);
					return -1;
				} else if (result > 0) {
					assert(_cmp(rhs, lhs) < 0);
					return 1;
				}
				assert(_cmp(rhs, lhs) == 0);
				return 0;
			} else {
				if (_cmp(lhs, rhs)) {
					assert(!_cmp(rhs, lhs));
					return -1;
				}
				if (_cmp(rhs, lhs))
					return 1;
				return 0;
			}
		}

		/// @brief Perform a fallible three-way comparison through the comparator.
		/// @tparam L The type of the left-hand operand.
		/// @tparam R The type of the right-hand operand.
		/// @param lhs The left-hand operand.
		/// @param rhs The right-hand operand.
		/// @return The comparison result, or peff::PARTIAL_UNORDERED when the comparison failed,
		///         that is, neither "result > 0", "result < 0" nor "result == 0" holds.
		template <typename L, typename R>
		PEFF_FORCEINLINE PartialOrd _compare_partial(const L &lhs, const R &rhs) const noexcept {
			const PartialOrd result = _cmp(lhs, rhs);

			if (result < 0)
				return PARTIAL_LT;
			if (result > 0)
				return PARTIAL_GT;
			if (result == 0)
				return PARTIAL_EQ;

			return PARTIAL_UNORDERED;
		}

		/// @brief Locate the lower bound of a key within a node.
		/// @tparam U The type of the key to search for.
		/// @param node The node to search in.
		/// @param key The key to search for.
		/// @param idx_out Receives the index of the key when it is present, or of its insertion
		///        point otherwise.
		/// @param found_out Receives true when the key is present in the node.
		/// @return false when the comparison failed, true otherwise.
		template <typename U>
		PEFF_FORCEINLINE bool _bsearch_lower_bound(const Node *node, const U &key, size_t &idx_out, bool &found_out) const noexcept {
			size_t lo = 0, hi = node->num_keys;
			while (lo < hi) {
				size_t mid = lo + (hi - lo) / 2;

				if constexpr (_is_partial_ordered) {
					const PartialOrd result = _compare_partial(key, node->key(mid));

					if (result < 0)
						hi = mid;
					else if (result > 0)
						lo = mid + 1;
					else if (result == 0) {
						idx_out = mid;
						found_out = true;
						return true;
					} else
						return false;
				} else {
					int result = _compare(key, node->key(mid));

					if (result < 0)
						hi = mid;
					else if (result > 0)
						lo = mid + 1;
					else {
						idx_out = mid;
						found_out = true;
						return true;
					}
				}
			}
			idx_out = lo;
			found_out = false;
			return true;
		}

		/// @brief Find a key within the subtree rooted at a node.
		/// @tparam U The type of the key to find.
		/// @param key The key to find.
		/// @param node The node to start the search from.
		/// @param node_out Receives the node that contains the key, or null when it is not present.
		/// @param idx_out Receives the index of the key within the node.
		/// @return false when the comparison failed, true otherwise.
		template <typename U>
		inline bool _find(const U &key, Node *node, Node *&node_out, size_t &idx_out) const noexcept {
		recurse:
			size_t i;
			bool found;

			if (!_bsearch_lower_bound(node, key, i, found))
				return false;

			if (found) {
				node_out = node;
				idx_out = i;
				return true;
			}

			if (node->is_leaf()) {
				node_out = nullptr;
				return true;
			}

			node = node->child_node(i);
			goto recurse;
		}

		/// @brief Find the greatest key that is not greater than the given key within a subtree.
		/// @tparam U The type of the key to query.
		/// @param key The key to query.
		/// @param node The node to start the search from.
		/// @param node_out Receives the node that contains the key, or null when there is none.
		/// @param idx_out Receives the index of the key within the node.
		/// @return false when the comparison failed, true otherwise.
		template <typename U>
		PEFF_FORCEINLINE bool _find_max_lteq(const U &key, Node *node, Node *&node_out, size_t &idx_out) const noexcept {
			node_out = nullptr;
			idx_out = 0;

			Node *cur = node;

			while (cur) {
				size_t i;
				bool found;

				if (!_bsearch_lower_bound(cur, key, i, found))
					return false;

				if (found) {
					node_out = cur;
					idx_out = i;
					return true;
				}

				// Every key from the insertion point on is greater than the queried one, so the
				// best candidate within this node is the key right before it.
				if (i > 0) {
					node_out = cur;
					idx_out = i - 1;
				}

				if (cur->is_leaf())
					return true;

				cur = cur->child_node(i);
			}

			return true;
		}

		PEFF_FORCEINLINE static Node *_min_node(Node *node) noexcept {
			while (node->num_children)
				node = node->children[0];
			return node;
		}

		PEFF_FORCEINLINE static Node *_max_node(Node *node) noexcept {
			while (node->num_children)
				node = node->children[node->num_children - 1];
			return node;
		}

		PEFF_FORCEINLINE static std::pair<Node *, size_t> _predecessor(Node *node, size_t index) noexcept {
			Node *cur = node->children[index];
			while (!cur->is_leaf())
				cur = cur->children[cur->num_keys];
			return { cur, cur->num_keys - 1 };
		}

		PEFF_FORCEINLINE static std::pair<Node *, size_t> _successor(Node *node, size_t index) noexcept {
			Node *cur = node->children[index + 1];
			while (!cur->is_leaf())
				cur = cur->children[0];
			return { cur, 0 };
		}

		PEFF_FORCEINLINE bool _split(Node *x, size_t index) noexcept {
			Node *y = x->child_node(index);
			Node *z;

			if (!(z = _alloc_node()))
				return false;

			/*peff::ScopeGuard sg([this, z]() noexcept {
				_dealloc_node(z);
			});*/

			// Move right half of key array into new node.
			for (size_t j = 0; j < Degree - 1; ++j)
				z->push_key(std::move(y->key(j + Degree)));

			// Move right half of the child array into new node.
			if (!y->is_leaf()) {
				for (size_t j = 0; j < Degree; ++j) {
					Node *child = y->child_node(j + Degree);
					z->push_child_node(y->child_node(j + Degree));
				}
				y->num_children -= Degree;
			}

			x->insert_key(index, std::move(y->key(Degree - 1)));
			x->insert_child_node(index + 1, z);

			y->num_keys -= Degree;

			x->has_undo = true;
			x->undo_index = index;
			x->undo_new_sibling = z;

			return true;
		}

		PEFF_FORCEINLINE void _merge(Node *left_child, Node *right_child, Node *parent, size_t index) noexcept {
			left_child->push_key(std::move(parent->key(index)));

			for (size_t i = 0; i < Degree - 1; ++i) {
				left_child->push_key(std::move(right_child->key(i)));
			}

			if (!left_child->is_leaf()) {
				for (size_t i = 0; i < Degree; ++i) {
					left_child->push_child_node(right_child->child_node(i));
				}
			}

			_dealloc_node(right_child);

			parent->remove_key(index);
			parent->remove_child_node(index + 1);

			if (!parent->num_keys) {
				if (parent->parent == nullptr) {
					_root = parent->child_node(0);
					_root->parent = nullptr;
					_dealloc_node(parent);
				} else {
					PEFF_ASSERT(false, "Non-root node should not underflow in deletion");
				}
			}
		}

		/// @brief Merge a node with its right sibling, moving the separator key down.
		/// @param left_child The left node that receives the keys.
		/// @param right_child The right node that is drained and released.
		/// @param parent The common parent of the two nodes.
		/// @param index The index of the separator key within the parent.
		/// @note Unlike _merge, the parent is allowed to underflow; the caller rebalances upwards.
		PEFF_FORCEINLINE void _merge_up(Node *left_child, Node *right_child, Node *parent, size_t index) noexcept {
			left_child->push_key(std::move(parent->key(index)));

			// A node being rebalanced upwards may hold fewer than Degree - 1 keys, so move exactly
			// as many keys and children as the right node actually has.
			const size_t right_keys = right_child->num_keys;
			for (size_t i = 0; i < right_keys; ++i)
				left_child->push_key(std::move(right_child->key(i)));

			if (!left_child->is_leaf()) {
				const size_t right_children = right_child->num_children;
				for (size_t i = 0; i < right_children; ++i)
					left_child->push_child_node(right_child->child_node(i));
			}

			_dealloc_node(right_child);

			parent->remove_key(index);
			parent->remove_child_node(index + 1);
		}

		/// @brief Restore the B-tree invariants after a key has been removed from a leaf.
		/// @param node The leaf a key has just been removed from.
		PEFF_FORCEINLINE void _rebalance_up(Node *node) noexcept {
			while (node && node->num_keys < Degree - 1) {
				Node *parent = static_cast<Node *>(node->parent);

				if (!parent) {
					// The root may hold fewer keys, but an empty root is collapsed.
					if (!node->num_keys) {
						if (node->is_leaf()) {
							_dealloc_node(node);
							_root = nullptr;
						} else {
							Node *new_root = node->child_node(0);
							new_root->parent = nullptr;
							_dealloc_node(node);
							_root = new_root;
						}
					}
					return;
				}

				size_t idx = 0;
				while (idx <= parent->num_keys && parent->child_node(idx) != node)
					++idx;

				PEFF_ASSERT(idx <= parent->num_keys, "The node must be a child of its parent");

				Node *left_sib = (idx > 0) ? parent->child_node(idx - 1) : nullptr;
				Node *right_sib = (idx < parent->num_keys) ? parent->child_node(idx + 1) : nullptr;

				if (left_sib && left_sib->num_keys >= Degree) {
					node->insert_key(0, std::move(parent->key(idx - 1)));
					if (!node->is_leaf())
						node->insert_child_node(0, left_sib->child_node(left_sib->num_children - 1));

					peff::move_assign_or_move_construct(parent->key(idx - 1), std::move(left_sib->key(left_sib->num_keys - 1)));
					left_sib->remove_key(left_sib->num_keys - 1);

					if (!left_sib->is_leaf())
						left_sib->remove_child_node(left_sib->num_children - 1);

					return;
				}

				if (right_sib && right_sib->num_keys >= Degree) {
					node->push_key(std::move(parent->key(idx)));
					if (!node->is_leaf())
						node->push_child_node(right_sib->child_node(0));

					peff::move_assign_or_move_construct(parent->key(idx), std::move(right_sib->key(0)));
					right_sib->remove_key(0);

					if (!right_sib->is_leaf())
						right_sib->remove_child_node(0);

					return;
				}

				if (left_sib)
					_merge_up(left_sib, node, parent, idx - 1);
				else if (right_sib)
					_merge_up(node, right_sib, parent, idx);
				else {
					PEFF_ASSERT(false, "A non-root node must have a sibling");
					return;
				}

				node = parent;
			}
		}

		/// @brief Extract the key at a position and restore the tree upwards.
		/// @param node The node that contains the key.
		/// @param idx The index of the key within the node.
		/// @return The extracted key.
		PEFF_FORCEINLINE T _extract(Node *node, size_t idx) noexcept {
			T removed = std::move(node->key(idx));

			if (!node->is_leaf()) {
				// Replace the key with its predecessor, which always lives in a leaf.
				auto pred = _predecessor(node, idx);
				peff::move_assign_or_move_construct(node->key(idx), std::move(pred.first->key(pred.second)));
				node = pred.first;
				idx = pred.second;
			}

			node->remove_key(idx);
			--_num_elements;
			_rebalance_up(node);

			return removed;
		}

		PEFF_FORCEINLINE void _rollback_insert(Node *start_node) {
			Node *cur = start_node;
			while (cur) {
				Node *parent = static_cast<Node *>(cur->parent);
				if (cur->has_undo) {
					cur->has_undo = false;
					Node *y = cur->child_node(cur->undo_index);
					Node *z = cur->undo_new_sibling;
					size_t cur_num_keys = cur->num_keys;
					_merge(y, z, cur, cur->undo_index);
					if (!parent)
						break;
				}
				cur = parent;
			}
		}

		/// @brief Insert a key into a node that is guaranteed to have room for it.
		/// @param node The node to start the insertion from.
		/// @param key The key to be inserted, must be an rvalue.
		/// @return The pointer to the inserted key. On failure it is either a null pointer (out of
		///         memory) or, in the fallible mode, an Unordered error when the comparison failed.
		///         Any structural change performed on the way down is rolled back before returning.
		PEFF_FORCEINLINE Outcome<T *> _insert_nonfull(Node *node, T &&key) noexcept {
		recurse:
			size_t i;
			bool found;

			if (!_bsearch_lower_bound(node, key, i, found)) {
				// The comparison failed: roll back every split performed on the way down.
				_rollback_insert(node);
				if constexpr (Fallible)
					return Unordered();
				else
					return nullptr;
			}

			if (node->is_leaf()) {
				node->insert_key(i, std::move(key));

				{
					Node *cur = static_cast<Node *>(node->parent);
					while (cur) {
						cur->has_undo = false;
						cur = static_cast<Node *>(cur->parent);
					}
				}

				return &node->key(i);
			}

			if (node->child_node(i)->num_keys == 2 * Degree - 1) {
				if (!_split(node, i)) {
					_rollback_insert(node);
					return nullptr;
				}

				if constexpr (_is_partial_ordered) {
					const PartialOrd result = _compare_partial(key, node->key(i));

					if (result > 0)
						++i;
					else if (!(result < 0) && !(result == 0)) {
						// The comparison failed: roll the split back and report it.
						_rollback_insert(node);
						return Unordered();
					}
				} else if (_gt(key, node->key(i)))
					++i;
			}
			node = node->child_node(i);
			goto recurse;
		}

		/// @brief Whether an insertion outcome stands for a failure.
		/// @param result The outcome to check.
		[[nodiscard]] PEFF_FORCEINLINE static bool _is_insert_failure(const Outcome<T *> &result) noexcept {
			if constexpr (Fallible)
				return result.is_error() || !result.value();
			else
				return !result;
		}

		PEFF_FORCEINLINE T delete_max(Node *node) {
		recurse:
			if (node->is_leaf()) {
				T max_key = std::move(node->key(node->num_keys - 1));
				node->remove_key(node->num_keys - 1);
				if (!node->num_keys) {
					_dealloc_node(node);
					_root = nullptr;
				}
				return max_key;
			}

			Node *right_most = node->child_node(node->num_keys);
			if (right_most->num_keys == Degree - 1) {
				Node *left_sib = node->child_node(node->num_keys - 1);
				if (left_sib->num_keys >= Degree) {
					right_most->insert_key(0, std::move(node->key(node->num_keys - 1)));
					if (!right_most->is_leaf()) {
						right_most->insert_child_node(0, left_sib->child_node(left_sib->num_children - 1));
					}
					peff::move_assign_or_move_construct(node->key(node->num_keys - 1), std::move(left_sib->key(left_sib->num_keys - 1)));
					left_sib->remove_key(left_sib->num_keys - 1);
					if (!left_sib->is_leaf()) {
						left_sib->remove_child_node(left_sib->num_children - 1);
					}
				} else {
					_merge(left_sib, right_most, node, node->num_keys - 1);
					right_most = left_sib;
				}
			}
			node = right_most;
			goto recurse;
		}

		PEFF_FORCEINLINE T delete_min(Node *node) {
		recurse:
			if (node->is_leaf()) {
				T min_key = std::move(node->key(0));
				node->remove_key(0);
				return min_key;
			}

			Node *left_most = node->child_node(0);
			if (left_most->num_keys == Degree - 1) {
				Node *right_sib = node->child_node(1);
				if (right_sib->num_keys >= Degree) {
					left_most->push_key(std::move(node->key(0)));
					if (!left_most->is_leaf()) {
						left_most->push_child_node(right_sib->child_node(0));
					}
					peff::move_assign_or_move_construct(node->key(0), std::move(right_sib->key(0)));
					right_sib->remove_key(0);
					if (!right_sib->is_leaf()) {
						right_sib->remove_child_node(0);
					}
				} else {
					_merge(left_most, right_sib, node, 0);
				}
			}
			node = left_most;
			goto recurse;
		}

	public:
		PEFF_FORCEINLINE BTreeSet(PEFF_NONNULL peff::Alloc *allocator, Comparator cmp = {}) : _allocator(allocator), _cmp(cmp) {
		}

		PEFF_FORCEINLINE BTreeSet(ThisType &&other) noexcept : _allocator(std::move(other._allocator)), _cmp(std::move(other._cmp)), _root(other._root), _num_elements(other._num_elements) {
			other._root = nullptr;
			other._num_elements = 0;
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
			if (this == &rhs)
				return *this;

			clear();

			_allocator = std::move(rhs._allocator);
			_cmp = std::move(rhs._cmp);
			_root = rhs._root;
			_num_elements = rhs._num_elements;

			rhs._root = nullptr;
			rhs._num_elements = 0;

			return *this;
		}

		PEFF_FORCEINLINE ~BTreeSet() noexcept {
			_destroy_tree();
		}

		inline Outcome<T *> insert(T &&key) noexcept {
			if (!_root) {
				Node *new_node = _alloc_node();

				if (!new_node)
					return nullptr;

				peff::ScopeGuard sg([this, new_node]() noexcept {
					_dealloc_node(new_node);
				});

				Outcome<T *> result = _insert_nonfull(new_node, std::move(key));

				if (_is_insert_failure(result))
					return result;

				sg.release();

				_root = new_node;
				++_num_elements;
				return result;
			}

			PEFF_ASSERT(_root->num_keys <= 2 * Degree - 1, "Invalid key number detected");

			if (_root->num_keys == 2 * Degree - 1) {
				Node *new_node = _alloc_node();

				if (!new_node)
					return nullptr;

				new_node->push_child_node(_root);

				peff::ScopeGuard sg([this, new_node]() noexcept {
					_dealloc_node(new_node);
					_root->parent = nullptr;
				});

				if (!_split(new_node, 0))
					return nullptr;

				peff::ScopeGuard undo_sg([this, new_node]() noexcept {
					_rollback_insert(new_node);
				});

				Outcome<T *> result = _insert_nonfull(new_node, std::move(key));

				if (_is_insert_failure(result))
					return result;

				undo_sg.release();
				sg.release();

				{
					Node *cur = static_cast<Node *>(new_node->parent);
					while (cur) {
						cur->has_undo = false;
						cur = static_cast<Node *>(cur->parent);
					}
				}

				_root = new_node;
				++_num_elements;
				return result;
			}

			Outcome<T *> result = _insert_nonfull(_root, std::move(key));

			if (_is_insert_failure(result))
				return result;

			++_num_elements;
			return result;
		}

		/// @brief Remove an element from the set.
		/// @param key The key of the element to be removed.
		/// @return The removed element, an empty option when there is no such element, or, in the
		///         fallible mode, an Unordered error when the comparison failed. In the latter two
		///         cases the tree is left unmodified.
		inline Outcome<Option<T>> remove(const T &key) noexcept {
			return remove_alt<T>(key);
		}

		/// @brief Remove an element from the set, using an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key The key of the element to be removed.
		/// @return The removed element, an empty option when there is no such element, or, in the
		///         fallible mode, an Unordered error when the comparison failed. In the latter two
		///         cases the tree is left unmodified.
		template <typename U>
		inline Outcome<Option<T>> remove_alt(const U &key) noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (_is_partial_ordered) {
				// A failed comparison must leave the tree untouched, so the whole descent is done
				// before any structural change takes place.
				Node *found_node = nullptr;
				size_t found_idx = 0;

				if (_root && !_find(key, _root, found_node, found_idx))
					return Unordered();

				if (!found_node)
					return Option<T>(NULLOPT);

				return Option<T>(std::move(_extract(found_node, found_idx)));
			} else {
				Node *node = _root;

			recurse:
				if (!node)
					return Option<T>(NULLOPT);

				size_t idx;
				bool found;

				_bsearch_lower_bound(node, key, idx, found);

				if (found) {
					if (node->is_leaf()) {
						T removed = std::move(node->key(idx));
						node->remove_key(idx);
						--_num_elements;

						if (!node->num_keys) {
							_dealloc_node(node);
							_root = nullptr;
						}
						return Option<T>(std::move(removed));
					}

					Node *left_child = node->child_node(idx);
					Node *right_child = node->child_node(idx + 1);

					if (left_child->num_keys >= Degree) {
						T removed = std::move(node->key(idx));
						peff::move_assign_or_move_construct(node->key(idx), delete_max(left_child));
						--_num_elements;
						return Option<T>(std::move(removed));
					}
					if (right_child->num_keys >= Degree) {
						T removed = std::move(node->key(idx));
						peff::move_assign_or_move_construct(node->key(idx), delete_min(right_child));
						--_num_elements;
						return Option<T>(std::move(removed));
					}
					_merge(left_child, right_child, node, idx);
					node = left_child;
					goto recurse;
				}

				if (node->is_leaf())
					return Option<T>(NULLOPT);

				Node *child = node->child_node(idx);

				PEFF_ASSERT(child, "child should be non-null!");

				if (child->num_keys == Degree - 1) {
					Node *left_sib = (idx > 0) ? node->child_node(idx - 1) : nullptr;
					Node *right_sib = (idx < node->num_keys) ? node->child_node(idx + 1) : nullptr;

					if (left_sib && left_sib->num_keys >= Degree) {
						child->insert_key(0, std::move(node->key(idx - 1)));
						if (!child->is_leaf())
							child->insert_child_node(0, left_sib->child_node(left_sib->num_children - 1));

						peff::move_assign_or_move_construct(node->key(idx - 1), std::move(left_sib->key(left_sib->num_keys - 1)));
						left_sib->remove_key(left_sib->num_keys - 1);
						if (!left_sib->is_leaf())
							left_sib->remove_child_node(left_sib->num_children - 1);
					} else if (right_sib && right_sib->num_keys >= Degree) {
						child->push_key(std::move(node->key(idx)));
						if (!child->is_leaf())
							child->push_child_node(right_sib->child_node(0));

						peff::move_assign_or_move_construct(node->key(idx), std::move(right_sib->key(0)));
						right_sib->remove_key(0);
						if (!right_sib->is_leaf())
							right_sib->remove_child_node(0);
					} else {
						if (left_sib) {
							_merge(left_sib, child, node, idx - 1);
							child = left_sib;
						} else if (right_sib) {
							_merge(child, right_sib, node, idx);
						}
					}
				}
				node = child;
				goto recurse;
			}
		}

		/// @brief Get reference to the element that is associated to a key.
		/// @param key Key to the element.
		/// @return Reference to the stored element, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		PEFF_FORCEINLINE AtResult at(const T &key) noexcept {
			return at_alt<T>(key);
		}

		/// @brief Get reference to the element that is associated to an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key to the element.
		/// @return Reference to the stored element, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE AtResult at_alt(const U &key) noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (Fallible) {
				Node *node = nullptr;
				size_t idx = 0;

				if (_root && !_find(key, _root, node, idx))
					return Unordered();

				PEFF_ASSERT(node, "Corresponding element not found");

				return node ? &node->key(idx) : nullptr;
			} else {
				PEFF_ASSERT(_root, "The set is empty");

				Node *node = nullptr;
				size_t idx = 0;
				_find(key, _root, node, idx);

				PEFF_ASSERT(node, "Corresponding element not found");

				return node->key(idx);
			}
		}

		/// @brief Get immutable reference to the element that is associated to a key.
		/// @param key Key to the element.
		/// @return Immutable reference to the stored element, or, in the fallible mode, an Unordered
		///         error when the comparison failed.
		PEFF_FORCEINLINE ConstAtResult at(const T &key) const noexcept {
			return at_alt<T>(key);
		}

		/// @brief Get immutable reference to the element that is associated to an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key to the element.
		/// @return Immutable reference to the stored element, or, in the fallible mode, an Unordered
		///         error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE ConstAtResult at_alt(const U &key) const noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (Fallible) {
				Node *node = nullptr;
				size_t idx = 0;

				if (_root && !_find(key, _root, node, idx))
					return Unordered();

				PEFF_ASSERT(node, "Corresponding element not found");

				return node ? &node->key(idx) : nullptr;
			} else {
				PEFF_ASSERT(_root, "The set is empty");

				Node *node = nullptr;
				size_t idx = 0;
				_find(key, _root, node, idx);

				PEFF_ASSERT(node, "Corresponding element not found");

				return node->key(idx);
			}
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _num_elements;
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return !_num_elements;
		}

		/// @brief Release every element of the set.
		PEFF_FORCEINLINE void clear() noexcept {
			_destroy_tree();
		}

		/// @brief Get the allocator of the container.
		/// @return Allocator of the container.
		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const noexcept {
			return _allocator.get();
		}

		/// @brief Replace the allocator of the container.
		/// @param rhs Allocator for replacement.
		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
			verify_replaceable(_allocator.get(), rhs);
			_allocator = rhs;
		}

		/// @brief Get the comparator of the container.
		/// @return Reference to the comparator of the container.
		PEFF_FORCEINLINE Comparator &comparator() noexcept {
			return _cmp;
		}

		/// @brief Get the immutable comparator of the container.
		/// @return Immutable reference to the comparator of the container.
		PEFF_FORCEINLINE const Comparator &comparator() const noexcept {
			return _cmp;
		}

		/// @brief Verify the invariants of the B-tree.
		PEFF_FORCEINLINE void verify() const noexcept {
			if (!_root)
				return;

			PEFF_ASSERT(_root->parent == nullptr, "The root must not have a parent");

			size_t leaf_depth = (size_t)-1;
			size_t total = 0;

			// The traversal is iterative so that the call stack depth never depends on the tree height.
			const Node *cur = _root;
			size_t depth = 0;
			size_t next_child = 0;
			bool entered = true;

			while (cur) {
				if (entered) {
					PEFF_ASSERT(cur->num_keys <= 2 * Degree - 1, "Key capacity exceeded");

					if (cur->parent)
						PEFF_ASSERT(cur->num_keys >= Degree - 1, "Key number underflow");

					PEFF_ASSERT(
						(cur->is_leaf() && cur->num_children == 0) || cur->num_children == cur->num_keys + 1,
						"Invalid child number");

					for (size_t i = 1; i < cur->num_keys; ++i) {
						if constexpr (_is_partial_ordered) {
							const PartialOrd result = _compare_partial(cur->key(i - 1), cur->key(i));
							PEFF_ASSERT(result < 0, "The keys must be strictly ascending");
						} else {
							PEFF_ASSERT(_lt(cur->key(i - 1), cur->key(i)), "The keys must be strictly ascending");
						}
					}

					total += cur->num_keys;

					if (cur->is_leaf()) {
						if (leaf_depth == (size_t)-1)
							leaf_depth = depth;
						else
							PEFF_ASSERT(leaf_depth == depth, "Every leaf must be on the same depth");
					}
				}

				if (next_child < cur->num_children) {
					cur = cur->child_node(next_child);
					++depth;
					next_child = 0;
					entered = true;
				} else {
					const Node *parent = static_cast<const Node *>(cur->parent);

					if (!parent) {
						PEFF_ASSERT(depth == 0, "The traversal must end at the root");
						break;
					}

					size_t idx = 0;
					while (idx < parent->num_children && parent->child_node(idx) != cur)
						++idx;

					PEFF_ASSERT(idx < parent->num_children, "The node must be a child of its parent");

					cur = parent;
					--depth;
					next_child = idx + 1;
					entered = false;
				}
			}

			PEFF_ASSERT(total == _num_elements, "The number of elements mismatch");
		}

		PEFF_FORCEINLINE Outcome<bool> contains(const T &key) const noexcept {
			return contains_alt<T>(key);
		}

		/// @brief Check if an element is already contained in the set, using an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key of the element to be checked.
		/// @return true when the element is contained, false otherwise, or, in the fallible mode, an
		///         Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<bool> contains_alt(const U &key) const noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (Fallible) {
				Node *node = nullptr;
				size_t idx = 0;

				if (_root && !_find(key, _root, node, idx))
					return Unordered();

				return node != nullptr;
			} else {
				if (!_root)
					return false;

				Node *node = nullptr;
				size_t idx = 0;
				_find(key, _root, node, idx);

				return node != nullptr;
			}
		}

		class Iterator {
		private:
			Node *_node;
			size_t _idx;
			bool _reversed;
			Node *_root;

			friend ThisType;

			/// @brief Move to the next element in ascending order.
			PEFF_FORCEINLINE void _step_forward() noexcept {
				if (!_node->is_leaf()) {
					Node *child = _node->child_node(_idx + 1);
					while (!child->is_leaf())
						child = child->child_node(0);
					_node = child;
					_idx = 0;
					return;
				}

				if (_idx + 1 < _node->num_keys) {
					++_idx;
					return;
				}

				Node *child = _node;
				Node *parent = static_cast<Node *>(child->parent);
				while (parent != nullptr) {
					size_t i = 0;
					while (i <= parent->num_keys && parent->child_node(i) != child)
						++i;
					if (i < parent->num_keys) {
						_node = parent;
						_idx = i;
						return;
					}
					child = parent;
					parent = static_cast<Node *>(parent->parent);
				}
				_node = nullptr;
				_idx = 0;
			}

			/// @brief Move to the next element in descending order.
			PEFF_FORCEINLINE void _step_backward() noexcept {
				if (!_node->is_leaf()) {
					Node *child = _node->child_node(_idx);
					while (!child->is_leaf())
						child = child->child_node(child->num_children - 1);
					_node = child;
					_idx = child->num_keys - 1;
					return;
				}

				if (_idx > 0) {
					--_idx;
					return;
				}

				Node *child = _node;
				Node *parent = static_cast<Node *>(child->parent);
				while (parent != nullptr) {
					size_t i = 0;
					while (i <= parent->num_keys && parent->child_node(i) != child)
						++i;
					if (i > 0) {
						_node = parent;
						_idx = i - 1;
						return;
					}
					child = parent;
					parent = static_cast<Node *>(parent->parent);
				}
				_node = nullptr;
				_idx = 0;
			}

		public:
			PEFF_FORCEINLINE Iterator(Node *node, size_t idx, bool reversed = false, Node *root = nullptr) : _node(node), _idx(idx), _reversed(reversed), _root(root) {}

			PEFF_FORCEINLINE const T &operator*() const {
				return _node->key(_idx);
			}

			PEFF_FORCEINLINE T *operator->() const noexcept {
				return &_node->key(_idx);
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				if (_node) {
					if (_reversed)
						_step_backward();
					else
						_step_forward();
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

			PEFF_FORCEINLINE IteratorDirection direction() const noexcept {
				return _reversed ? IteratorDirection::Reversed : IteratorDirection::Forward;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (_node) {
					if (_reversed)
						_step_forward();
					else
						_step_backward();
				} else if (_root) {
					if (_reversed) {
						_node = _min_node(_root);
						_idx = 0;
					} else {
						_node = _max_node(_root);
						_idx = _node->num_keys - 1;
					}
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

			PEFF_FORCEINLINE bool operator==(const Iterator &other) const {
				return _node == other._node && _idx == other._idx;
			}
			PEFF_FORCEINLINE bool operator!=(const Iterator &other) const {
				return !(*this == other);
			}
		};

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
				return _iterator.direction();
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

		PEFF_FORCEINLINE Iterator begin() noexcept {
			if (!_root)
				return end();
			return Iterator(_min_node(_root), 0, false, _root);
		}

		PEFF_FORCEINLINE Iterator end() noexcept {
			return Iterator(nullptr, 0, false, _root);
		}

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			if (!_root)
				return end();
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}

		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return ConstIterator(Iterator(nullptr, 0, false, const_cast<ThisType *>(this)->_root));
		}

		PEFF_FORCEINLINE Iterator rbegin() noexcept {
			if (!_root)
				return rend();

			Node *node = _max_node(_root);
			return Iterator(node, node->num_keys - 1, true, _root);
		}

		PEFF_FORCEINLINE Iterator rend() noexcept {
			return Iterator(nullptr, 0, true, _root);
		}

		PEFF_FORCEINLINE ConstIterator crbegin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rbegin());
		}

		PEFF_FORCEINLINE ConstIterator cend_reversed() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->rend());
		}

		PEFF_FORCEINLINE Outcome<Iterator> find(const T &key) noexcept {
			return find_alt<T>(key);
		}

		/// @brief Find an element in the set, using an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key of the element to be found.
		/// @return Iterator to the element, an end iterator when it is not present, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_alt(const U &key) noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (Fallible) {
				Node *node = nullptr;
				size_t idx = 0;

				if (_root && !_find(key, _root, node, idx))
					return Unordered();

				if (!node)
					return end();
				return Iterator(node, idx, false, _root);
			} else {
				if (!_root)
					return end();

				Node *node = nullptr;
				size_t idx = 0;
				_find(key, _root, node, idx);

				if (!node)
					return end();
				return Iterator(node, idx, false, _root);
			}
		}

		PEFF_FORCEINLINE Outcome<ConstIterator> find(const T &key) const noexcept {
			return find_alt<T>(key);
		}

		/// @brief Find an element in the set, using an alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key of the element to be found.
		/// @return Immutable iterator to the element, an end iterator when it is not present, or, in
		///         the fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_alt(const U &key) const noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if constexpr (Fallible) {
				Node *node = nullptr;
				size_t idx = 0;

				if (_root && !_find(key, _root, node, idx))
					return Unordered();

				if (!node)
					return end();
				return ConstIterator(Iterator(node, idx, false, _root));
			} else {
				if (!_root)
					return end();

				Node *node = nullptr;
				size_t idx = 0;
				_find(key, _root, node, idx);

				if (!node)
					return end();
				return ConstIterator(Iterator(node, idx, false, _root));
			}
		}

		/// @brief Find the element with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Iterator to the element, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq(const T &key) noexcept {
			return find_max_lteq_alt<T>(key);
		}

		/// @brief Find the element with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key for query.
		/// @return Iterator to the element, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq_alt(const U &key) noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if (!_root)
				return end();

			Node *node = nullptr;
			size_t idx = 0;

			if constexpr (Fallible) {
				if (!_find_max_lteq(key, _root, node, idx))
					return Unordered();
			} else {
				_find_max_lteq(key, _root, node, idx);
			}

			if (!node)
				return end();

			return Iterator(node, idx, false, _root);
		}

		/// @brief Find the element with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Immutable iterator to the element, an end iterator when there is none, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq(const T &key) const noexcept {
			return find_max_lteq_alt<T>(key);
		}

		/// @brief Find the element with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @tparam U The type of the key.
		/// @param key Key for query.
		/// @return Immutable iterator to the element, an end iterator when there is none, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq_alt(const U &key) const noexcept {
			static_assert(_is_alt_queryable<U>, "The query type is not comparable with the key");

			if (!_root)
				return end();

			Node *node = nullptr;
			size_t idx = 0;

			if constexpr (Fallible) {
				if (!_find_max_lteq(key, _root, node, idx))
					return Unordered();
			} else {
				_find_max_lteq(key, _root, node, idx);
			}

			if (!node)
				return end();

			return ConstIterator(Iterator(node, idx, false, _root));
		}

		/// @brief Remove the element pointed to by an iterator from the set.
		/// @param iterator Iterator to the element to be removed.
		/// @return The removed element.
		PEFF_FORCEINLINE Option<T> remove(const Iterator &iterator) noexcept {
			PEFF_ASSERT(iterator._node, "Cannot remove the end iterator");
			return Option<T>(std::move(_extract(iterator._node, iterator._idx)));
		}
	};
}

#endif
