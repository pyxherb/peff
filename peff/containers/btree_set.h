#ifndef _PEFF_CONTAINERS_BTREE_SET_H_
#define _PEFF_CONTAINERS_BTREE_SET_H_

#include "basedefs.h"
#include <peff/base/misc.h>
#include <peff/utils/misc.h>
#include <peff/utils/option.h>
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

	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false, size_t Degree = _calc_btree_default_node_deg<T>()>
	class BTreeSet final {
	private:
		using ThisType = BTreeSet<T, Comparator, IsThreeway, Degree>;

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

		PEFF_FORCEINLINE bool _lt(const T &lhs, const T &rhs) const noexcept {
			if constexpr (IsThreeway) {
				return _cmp(lhs, rhs) < 0;
			} else {
				return _cmp(lhs, rhs);
			}
		}

		PEFF_FORCEINLINE bool _gt(const T &lhs, const T &rhs) const noexcept {
			if constexpr (IsThreeway) {
				return _cmp(lhs, rhs) > 0;
			} else {
				return _cmp(rhs, lhs);
			}
		}

		PEFF_FORCEINLINE int _compare(const T &lhs, const T &rhs) const noexcept {
			if constexpr (IsThreeway) {
				return _cmp(lhs, rhs);
			} else {
				if (_cmp(lhs, rhs))
					return -1;
				if (_cmp(rhs, lhs))
					return 1;
				return 0;
			}
		}

		PEFF_FORCEINLINE size_t _bsearch_lower_bound(const Node *node, const T &key) const noexcept {
			size_t lo = 0, hi = node->num_keys;
			while (lo < hi) {
				size_t mid = lo + (hi - lo) / 2;
				int result = _compare(key, node->key(mid));
				if (result < 0)
					hi = mid;
				else if (result > 0)
					lo = mid + 1;
				else
					return mid;
			}
			return lo;
		}

		inline std::pair<Node *, size_t> _find(const T &key, Node *node) const noexcept {
		recurse:
			size_t i = _bsearch_lower_bound(node, key);
			if ((i < node->num_keys) && (!this->_compare(key, node->key(i)))) {
				return { node, i };
			} else if (node->is_leaf())
				return { nullptr, 0 };
			else {
				node = node->child_node(i);
				goto recurse;
			}
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

		PEFF_FORCEINLINE T *_insert_nonfull(Node *node, T &&key) noexcept {
		recurse:
			size_t i = _bsearch_lower_bound(node, key);

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
				if (_gt(key, node->key(i)))
					++i;
			}
			node = node->child_node(i);
			goto recurse;
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

		PEFF_FORCEINLINE ~BTreeSet() noexcept {
			if (!_root) return;

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
		}

		inline T *insert(T &&key) noexcept {
			T *ptr;

			if (!_root) {
				Node *new_node = _alloc_node();

				if (!new_node)
					return nullptr;

				peff::ScopeGuard sg([this, new_node]() noexcept {
					_dealloc_node(new_node);
				});

				if (!(ptr = _insert_nonfull(new_node, std::move(key))))
					return nullptr;

				sg.release();

				_root = new_node;
			} else {
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

					if (!(ptr = _insert_nonfull(new_node, std::move(key))))
						return nullptr;

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
				} else {
					if (!(ptr = _insert_nonfull(_root, std::move(key))))
						return nullptr;
				}
			}

			++_num_elements;

			return ptr;
		}

		inline void remove(const T &key) noexcept {
			Node *node = _root;
		recurse:
			if (!node)
				return;

			size_t idx = _bsearch_lower_bound(node, key);

			if (idx < node->num_keys && !_compare(key, node->key(idx))) {
				if (node->is_leaf()) {
					node->remove_key(idx);
					--_num_elements;

					if (!node->num_keys) {
						_dealloc_node(node);
						_root = nullptr;
					}
					return;
				}

				Node *left_child = node->child_node(idx);
				Node *right_child = node->child_node(idx + 1);
				if (left_child->num_keys >= Degree) {
					peff::move_assign_or_move_construct(node->key(idx), delete_max(left_child));
					--_num_elements;
					return;
				}
				if (right_child->num_keys >= Degree) {
					peff::move_assign_or_move_construct(node->key(idx), delete_min(right_child));
					--_num_elements;
					return;
				}
				_merge(left_child, right_child, node, idx);
				node = left_child;
				goto recurse;
			}

			if (node->is_leaf())
				return;

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

		PEFF_FORCEINLINE T &at(const T &key) noexcept {
			PEFF_ASSERT(_root, "The set is empty");
			auto result = _find(key, _root);
			PEFF_ASSERT(!result.first, "Corresponding element not found");
			return result.first->key(result.second);
		}

		PEFF_FORCEINLINE const T &at(const T &key) const noexcept {
			PEFF_ASSERT(_root, "The set is empty");
			auto result = _find(key, _root);
			PEFF_ASSERT(!result.first, "Corresponding element not found");
			return result.first->key(result.second);
		}

		PEFF_FORCEINLINE bool contains(const T& key) const noexcept {
			if (!_root)
				return false;
			auto result = _find(key, _root);
			return result.first;
		}

		class Iterator {
		private:
			Node *_node;
			size_t _idx;

		public:
			PEFF_FORCEINLINE Iterator(Node *node, size_t idx) : _node(node), _idx(idx) {}

			PEFF_FORCEINLINE const T &operator*() const {
				return _node->key(_idx);
			}

			PEFF_FORCEINLINE T *operator->() const noexcept {
				return &_node->key(_idx);
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				if (!_node)
					return *this;

				if (!_node->is_leaf()) {
					Node *child = _node->child_node(_idx + 1);
					while (!child->is_leaf())
						child = child->child_node(0);
					_node = child;
					_idx = 0;
				} else {
					if (_idx + 1 < _node->num_keys) {
						++_idx;
					} else {
						Node *child = _node;
						Node *parent = static_cast<Node *>(child->parent);
						while (parent != nullptr) {
							size_t i = 0;
							while (i <= parent->num_keys && parent->child_node(i) != child)
								++i;
							if (i < parent->num_keys) {
								_node = parent;
								_idx = i;
								return *this;
							}
							child = parent;
							parent = static_cast<Node *>(parent->parent);
						}
						_node = nullptr;
						_idx = 0;
					}
				}
				return *this;
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

			PEFF_FORCEINLINE ConstIterator next() {
				ConstIterator iterator = *this;

				return ++iterator;
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
			return Iterator(_min_node(_root), 0);
		}

		PEFF_FORCEINLINE Iterator end() noexcept {
			return Iterator(nullptr, 0);
		}

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			if (!_root)
				return end();
			return ConstIterator(begin());
		}

		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return ConstIterator(Iterator(nullptr, 0));
		}

		PEFF_FORCEINLINE Iterator find(const T &key) noexcept {
			if (!_root)
				return end();
			auto result = _find(key, _root);
			if (!result.first)
				return end();
			return Iterator(result.first, result.second);
		}

		PEFF_FORCEINLINE ConstIterator find(const T &key) const noexcept {
			if (!_root)
				return end();
			auto result = _find(key, _root);
			if (!result.first)
				return end();
			return ConstIterator(Iterator(result.first, result.second));
		}
	};
}

#endif
