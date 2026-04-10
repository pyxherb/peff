#ifndef _PEFF_CONTAINERS_RADIX_TREE_H_
#define _PEFF_CONTAINERS_RADIX_TREE_H_

#include "basedefs.h"
#include "bitset.h"
#include <peff/utils/option.h>
#include <peff/utils/misc.h>
#include <peff/utils/bitops.h>
#include <peff/base/alloc.h>
#include <peff/base/scope_guard.h>
#include "misc.h"
#include <limits>
#include <stdexcept>

namespace peff {
	template <typename K, typename V>
	class RadixTree final {
	public:
		static_assert(std::is_integral_v<K>, "The key must be integral type");
		using ThisType = RadixTree<K, V>;

		constexpr static uintmax_t HEIGHT_MAX = std::numeric_limits<K>::digits;
		using Height = typename ::peff::AutoSizeUInteger<HEIGHT_MAX>::type;

		struct Node {
			Node *p = nullptr;
			Node *children[2] = {};
			OptionArray<V, 2> radix_value;
			Height height = 0;
			uint8_t num_used_children = 0;
			uint8_t offset = 0;

			PEFF_FORCEINLINE Node() {
			}
		};

	private:
		peff::RcObjectPtr<peff::Alloc> _allocator;
		Height _height;
		size_t _num_nodes;
		Node *_root;

		[[nodiscard]] PEFF_FORCEINLINE Node *_alloc_single_node() {
			Node *node = alloc_and_construct<Node>(_allocator.get(), alignof(Node));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _delete_single_node(Node *node) {
			destroy_and_release<Node>(_allocator.get(), node, alignof(Node));
		}

		[[nodiscard]] inline bool _grow_height(Height height) {
			Node *original_root = _root, *first_node = nullptr;
			Height original_height = _height;

			peff::ScopeGuard restore_guard([this, &first_node, original_root, original_height]() noexcept {
				for (Node *i = first_node; i; i = i->p) {
					_delete_single_node(i);
				}

				if (original_root)
					original_root->p = nullptr;

				_root = original_root;
				_height = original_height;
			});

			while (_height < height) {
				++_height;

				Node *new_node = _alloc_single_node();
				if (!new_node)
					return false;

				if (!first_node)
					first_node = new_node;

				new_node->height = _height;
				new_node->offset = 0;

				_root->p = new_node;
				new_node->children[0] = _root;
				++new_node->num_used_children;
				_root = new_node;
			}

			restore_guard.release();

			return true;
		}

		[[nodiscard]] inline bool _grow_node(K index, V &&data) {
			Node *node = _root;
			Height height = _height, shift;
			K offset;

			size_t i = 0;
			BitSet<sizeof(K) * 8 - 1> insertion_flags;

			peff::ScopeGuard sg([this, &insertion_flags, &i, &node]() noexcept {
				while (i && node) {
					--i;
					if (insertion_flags.get_bit(i)) {
						Node *parent = node->p;
						K offset = node->offset;
						--_num_nodes;
						_delete_single_node(node);
						if (parent) {
							parent->children[offset] = nullptr;
						}
						node = parent;
					} else {
						node = node->p;
					}
				}
			});

			while (height-- > 1) {
				shift = height;
				offset = (index >> shift) & 1;

				if ((!node->children[offset]) && (!node->radix_value.has_value(offset))) {
					Node *new_node = _alloc_single_node();
					if (!new_node)
						return false;
					new_node->height = height;
					++node->num_used_children;
					new_node->offset = offset;
					new_node->p = node;
					node->children[offset] = new_node;

					node = new_node;
					insertion_flags.set_bit(i);
				} else {
					assert(!node->radix_value.has_value(offset));
					node = node->children[offset];
				}
				++i;
			}

			sg.release();

			Node *leaf = node;
			offset = index & 1;
			if (leaf->children[offset] || leaf->radix_value.has_value(offset))
				// The key exists.
				std::terminate();
			leaf->radix_value.set_value(offset, std::move(data));
			return true;
		}

		[[nodiscard]] inline bool _grow_height_then_grow_node(K index, Height height, V &&data) {
			Node *original_root = _root, *first_node = nullptr;
			Height original_height = _height;

			peff::ScopeGuard restore_guard([this, &first_node, original_root, original_height]() noexcept {
				for (Node *i = first_node; i; i = i->p) {
					_delete_single_node(i);
				}

				if (original_root)
					original_root->p = nullptr;

				_root = original_root;
				_height = original_height;
			});

			while (_height < height) {
				++_height;

				Node *new_node = _alloc_single_node();
				if (!new_node)
					return false;

				if (!first_node)
					first_node = new_node;

				new_node->height = _height;
				new_node->offset = 0;

				_root->p = new_node;
				new_node->children[0] = _root;
				++new_node->num_used_children;
				_root = new_node;
			}

			if (!_grow_node(index, std::move(data)))
				return false;

			restore_guard.release();

			return true;
		}

		[[nodiscard]] inline bool _insert(K index, V &&data) {
			Node **node_ptr = &_root;
			Height height = index ? (Height)((sizeof(K) * 8 - 1 - count_leading_zero(index)) + 1) : 1;

			if (!_root) {
				Node *node = _alloc_single_node();
				if (!node)
					return false;
				peff::ScopeGuard delete_node_guard([this, node]() noexcept {
					_delete_single_node(node);
				});
				node->height = height;
				_height = height;

				Height original_height = _height;
				Node *original_root = _root;

				peff::ScopeGuard restore_props_guard([this, original_root, original_height]() noexcept {
					_root = original_root;
					_height = original_height;
				});

				_root = node;
				if (!_grow_node(index, std::move(data)))
					return false;
				restore_props_guard.release();
				delete_node_guard.release();
				return true;
			}

			if (height > _height)
				return _grow_height_then_grow_node(index, height, std::move(data));

			return _grow_node(index, std::move(data));
		}

		[[nodiscard]] inline V &_lookup(K index) const {
			if (!_root)
				std::terminate();
			Node *node = _root;
			Height height = _height;
			Height shift = height - 1;
			K offset;

			while (height) {
				offset = ((index >> shift) & 1);
				if (!node->children[offset]) {
					if (!node->radix_value.has_value(offset))
						std::terminate();
					return node->radix_value.value(offset);
				}
				node = node->children[offset];
				--shift;
				--height;
			}
		}

		[[nodiscard]] static inline Node *_get_min_node(Node *node) {
			assert(node);

			while (node->children[0] || node->children[1]) {
				node = node->children[0] ? node->children[0] : node->children[1];
			}

			return node;
		}

		[[nodiscard]] static inline Node *_get_max_node(Node *node) {
			assert(node);

			while (node->children[0] || node->children[1]) {
				node = node->children[1] ? node->children[1] : node->children[0];
			}

			return node;
		}

		[[nodiscard]] inline Node *_lookup_upper_bound(K index) const {
			if (!_root)
				return nullptr;

			Node *node = _root;
			Height shift = _height - 1;
			K offset;

			while (node && (node->height > 1)) {
				offset = (index >> shift) & 1;
				if (node->children[offset]) {
					node = node->children[offset];
					--shift;
				} else {
					if ((offset + 1 < 2) && node->children[offset + 1])
						return _get_min_node(node->children[offset + 1]);
					break;
				}
			}

			if (node && node->height == 1) {
				offset = index & 1;
				if (node->children[offset])
					return node->children[offset];
				if (offset + 1 < 2 && node->children[offset + 1])
					return node->children[offset + 1];
			}

			while (node && node->p) {
				K off = node->offset;
				node = node->p;
				if (off + 1 < 2 && node->children[off + 1]) {
					return _get_min_node(node->children[off + 1]);
				}
			}

			return nullptr;
		}

		[[nodiscard]] Node *delete_tree(K index) {
			Node *node = _root;
			Height height = _height;
			Height shift = height - 1;
			K offset;

			if (height < ((sizeof(K) * 8 - 1 - count_leading_zero(index)) + 1))
				return nullptr;

			while (height > 1) {
				if (!node)
					return nullptr;
				offset = (index >> shift) & 1;
				node = (Node *)node->children[offset];
				--shift;
				--height;
			}

			if (!node)
				return nullptr;

			offset = index & 1;
			Node *item = node->children[offset];
			if (!item)
				return nullptr;

			node->children[offset] = nullptr;
			--node->num_used_children;

			while ((!node->num_used_children) && (node != _root)) {
				Node *p = node->p;
				K off = node->offset;
				_delete_single_node(node);
				p->children[off] = nullptr;
				--p->num_used_children;
				node = p;
			}

			if (_root && (!_root->num_used_children)) {
				_delete_single_node(node);
				_root = nullptr;
				_height = 0;
			}

			return item;
		}

	public:
		PEFF_FORCEINLINE RadixTree(peff::Alloc *allocator) : _allocator(allocator), _height(0), _num_nodes(0), _root(nullptr) {}
		PEFF_FORCEINLINE ~RadixTree() {
			Node *cur_node = (Node *)_get_min_node(_root);
			Node *parent = (Node *)_root->p;
			bool walked_root_node = false;

			while (cur_node != parent) {
				if (cur_node->children[1]) {
					cur_node = (Node *)_get_min_node(cur_node->children[1]);
				} else {
					Node *node_to_delete = cur_node;

					while (cur_node->p && (cur_node == cur_node->p->children[1])) {
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

		[[nodiscard]] PEFF_FORCEINLINE bool insert(K key, V &&value) {
			return _insert(key, std::move(value));
		}

		PEFF_FORCEINLINE const V &at(K key) const {
			return _lookup(key);
		}

		PEFF_FORCEINLINE V &at(K key) {
			return _lookup(key);
		}

		PEFF_FORCEINLINE size_t size() {
			return _num_nodes;
		}

		PEFF_FORCEINLINE static Node *get_next_node(const Node *node) noexcept {
			while (true) {
				if (node->children[1]) {
					node = node->children[1];
					while (node->children[0]) {
						node = node->children[0];
					}
				} else {
					const Node *parent = node->p;
					while (parent && node == parent->children[1]) {
						node = parent;
						parent = parent->p;
					}
					node = parent;
				}

				if (!node) {
					return nullptr;
				}

				if (!node->children[0] && !node->children[1]) {
					return const_cast<Node *>(node);
				}
			}
		}

		PEFF_FORCEINLINE static Node *get_prev_node(const Node *node) noexcept {
			while (true) {
				if (node->children[0]) {
					node = node->children[0];
					while (node->children[1]) {
						node = node->children[1];
					}
				} else {
					const Node *parent = node->p;
					while (parent && node == parent->children[0]) {
						node = parent;
						parent = parent->p;
					}
					node = parent;
				}

				if (!node) {
					return nullptr;
				}

				if (!node->children[1] && !node->children[0]) {
					return const_cast<Node *>(node);
				}
			}
		}

		struct Iterator {
			Node *node;
			ThisType *tree;
			IteratorDirection direction;
			bool is_right;

			PEFF_FORCEINLINE Iterator(
				Node *node,
				ThisType *tree,
				IteratorDirection direction,
				bool is_right)
				: node(node),
				  tree(tree),
				  direction(direction),
				  is_right(is_right) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;
				is_right = it.is_right;

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
					if (!is_right) {
						if (node->radix_value.has_value(1)) {
							is_right = true;
							return *this;
						}
					}
					is_right = false;
					node = ThisType::get_next_node(node);
				} else {
					if (is_right) {
						if (node->radix_value.has_value(0)) {
							is_right = false;
							return *this;
						}
					}
					is_right = true;
					node = ThisType::get_prev_node(node);
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
					if (node == tree->_get_min_node(tree->_root))
						throw std::logic_error("Dereasing the begin iterator");

					if (!is_right) {
						if (node->radix_value.has_value(0)) {
							is_right = true;
							return *this;
						}
					}
					is_right = false;

					if (!node)
						node = (Node *)tree->_get_max_node(tree->_root);
					else
						node = ThisType::get_prev_node(node);
				} else {
					if (node == tree->_get_max_node(tree->_root))
						throw std::logic_error("Dereasing the begin iterator");

					if (is_right) {
						if (node->radix_value.has_value(0)) {
							is_right = false;
							return *this;
						}
					}
					is_right = true;

					if (!node)
						node = (Node *)tree->_get_min_node(tree->_root);
					else
						node = ThisType::get_next_node(node);
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
				if (node != it.node)
					return false;
				return is_right == it.is_right;
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
				if (node != it.node)
					return true;
				return is_right != it.is_right;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE V &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return is_right ? node->radix_value.value(1) : node->radix_value.value(0);
			}

			PEFF_FORCEINLINE V &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return is_right ? node->radix_value.value(1) : node->radix_value.value(0);
			}

			PEFF_FORCEINLINE V *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return is_right ? &node->radix_value.value(1) : &node->radix_value.value(0);
			}

			PEFF_FORCEINLINE V *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return is_right ? &node->radix_value.value(1) : &node->radix_value.value(0);
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			Node *node = _get_min_node(_root);
			bool is_right;
			if (node)
				is_right = node->radix_value.has_value(0) ? false : true;
			else
				is_right = false;
			return Iterator((Node *)node, this, IteratorDirection::Forward, is_right);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward, false);
		}
		PEFF_FORCEINLINE Iterator begin_reversed() {
			Node *node = _get_max_node(_root);
			bool is_right;
			if (node)
				is_right = node->radix_value.has_value(1) ? true : false;
			else
				is_right = false;
			return Iterator((Node *)node, this, IteratorDirection::Reversed, is_right);
		}
		PEFF_FORCEINLINE Iterator end_reversed() {
			return Iterator(nullptr, this, IteratorDirection::Reversed, true);
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

			PEFF_FORCEINLINE const V &operator*() {
				return *_iterator;
			}

			PEFF_FORCEINLINE const V &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE const V *operator->() {
				return &*_iterator;
			}

			PEFF_FORCEINLINE const V *operator->() const {
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
	};
}

#endif
