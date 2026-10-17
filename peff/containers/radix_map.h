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
	template <typename K, typename V, size_t R>
	class RadixMap final {
	public:
		static_assert(std::is_integral_v<K>, "The key must be integral type");
		using ThisType = RadixMap<K, V, R>;

		constexpr static uintmax_t HEIGHT_MAX = std::numeric_limits<K>::digits;
		using Height = typename ::peff::AutoSizeUInteger<HEIGHT_MAX>::type;

		constexpr static size_t N = static_cast<size_t>(1) << R;
		constexpr static K MASK = N - 1;

		constexpr static size_t MAX_STEPS = (HEIGHT_MAX + R - 1) / R;

		struct Node {
			Node *p = nullptr;
			Node *children[N] = {};
			OptionArray<V, N> radix_value;
			Height height = 0;
			size_t num_used_children = 0;
			K offset = 0;

			PEFF_FORCEINLINE Node() {
			}
		};

	private:
		peff::RcObjectPtr<peff::Alloc> _allocator;
		Height _height;
		size_t _size;
		Node *_root;

		[[nodiscard]] PEFF_FORCEINLINE PEFF_NULLABLE_RET Node *_alloc_single_node() {
			Node *node = alloc_and_construct<Node>(_allocator.get(), alignof(Node));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _delete_single_node(PEFF_NONNULL Node *node) {
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
			Height height = _height;

			size_t i = 0;
			BitSet<MAX_STEPS> insertion_flags;

			peff::ScopeGuard sg([this, &insertion_flags, &i, &node]() noexcept {
				while (i && node) {
					--i;
					if (insertion_flags.get_bit(i)) {
						Node *parent = node->p;
						K offset = node->offset;
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

			while (height > R) {
				Height shift = height - R;
				K offset = (index >> shift) & MASK;

				if ((!node->children[offset]) && (!node->radix_value.has_value(offset))) {
					Node *new_node = _alloc_single_node();
					if (!new_node)
						return false;
					new_node->height = height - R;
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
				height -= R;
			}

			sg.release();

			Node *leaf = node;
			K offset = index & MASK;
			if (leaf->children[offset] || leaf->radix_value.has_value(offset))
				std::terminate();
			leaf->radix_value.set_value(offset, std::move(data));
			++leaf->num_used_children;
			++_size;
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
				_height += R;

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

		static PEFF_FORCEINLINE Height _required_height(K index) {
			if (index == 0)
				return R;
			Height bits = (Height)(sizeof(K) * 8 - count_leading_zero(index));
			return ((bits + R - 1) / R) * R;
		}

		[[nodiscard]] inline bool _insert(K index, V &&data) {
			Height required_height = _required_height(index);

			if (!_root) {
				Node *node = _alloc_single_node();
				if (!node)
					return false;
				peff::ScopeGuard delete_node_guard([this, node]() noexcept {
					_delete_single_node(node);
				});
				node->height = required_height;
				_height = required_height;

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

			if (required_height > _height)
				return _grow_height_then_grow_node(index, required_height, std::move(data));

			return _grow_node(index, std::move(data));
		}

		[[nodiscard]] inline peff::Option<std::pair<Node *, size_t>> _lookup(K index) const {
			if (!_root)
				return NULLOPT;
			Node *node = _root;
			Height height = _height;
			while (height > R) {
				Height shift = height - R;
				K offset = (index >> shift) & MASK;
				if (!node->children[offset]) {
					if (!node->radix_value.has_value(offset))
						return NULLOPT;
					return std::pair<Node *, size_t>(node, offset);
				}
				node = node->children[offset];
				height -= R;
			}
			K offset = index & MASK;
			if (!node->radix_value.has_value(offset))
				return NULLOPT;
			return std::pair<Node *, size_t>(node, offset);
		}

		[[nodiscard]] static inline Node *_get_min_node(PEFF_NONNULL Node *node) {
			assert(node);

			while (true) {
				bool has_child = false;
				for (size_t i = 0; i < N; ++i) {
					if (node->children[i]) {
						node = node->children[i];
						has_child = true;
						break;
					}
				}
				if (!has_child)
					break;
			}

			return node;
		}

		[[nodiscard]] static inline Node *_get_max_node(PEFF_NONNULL Node *node) {
			assert(node);

			while (true) {
				bool has_child = false;
				for (size_t i = N; i-- > 0;) {
					if (node->children[i]) {
						node = node->children[i];
						has_child = true;
						break;
					}
				}
				if (!has_child)
					break;
			}

			return node;
		}

		[[nodiscard]] PEFF_NULLABLE_RET inline Node *_lookup_upper_bound(K index) const {
			if (!_root)
				return nullptr;

			Node *node = _root;
			Height height = _height;
			while (node && height > R) {
				Height shift = height - R;
				K offset = (index >> shift) & MASK;
				if (node->children[offset]) {
					node = node->children[offset];
					height -= R;
				} else {
					for (K i = offset + 1; i < N; ++i) {
						if (node->children[i])
							return _get_min_node(node->children[i]);
					}
					break;
				}
			}

			if (node && height == R) {
				K offset = index & MASK;
				for (K i = offset; i < N; ++i) {
					if (node->radix_value.has_value(i))
						return node;
				}

				while (node && node->p) {
					K off = node->offset;
					node = node->p;
					for (K i = off + 1; i < N; ++i) {
						if (node->children[i])
							return _get_min_node(node->children[i]);
					}
				}
			}

			return nullptr;
		}

	public:
		PEFF_FORCEINLINE RadixMap(PEFF_NONNULL peff::Alloc *allocator) noexcept : _allocator(allocator), _height(0), _size(0), _root(nullptr) {}
		PEFF_FORCEINLINE RadixMap(RadixMap &&rhs) noexcept : _allocator(std::move(rhs._allocator)), _height(rhs._height), _size(rhs._size), _root(rhs._root) {
			rhs._height = 0;
			rhs._size = 0;
			rhs._root = nullptr;
		}
		PEFF_FORCEINLINE ~RadixMap() {
			clear();
		}

		PEFF_FORCEINLINE RadixMap &operator=(RadixMap &&rhs) noexcept {
			verify_allocator(this->_allocator.get(), rhs._allocator.get());
			clear();
			this->_allocator = std::move(rhs._allocator);
			_height = rhs._height;
			_size = rhs._size;
			_root = rhs._root;
			rhs._height = 0;
			rhs._size = 0;
			rhs._root = nullptr;
			return *this;
		}

		inline void clear() noexcept {
			if (!_root)
				return;

			Node *cur_node = _get_min_node(_root);

			while (cur_node) {
				Node *child = nullptr;

				for (size_t i = 0; i < N; ++i) {
					if (cur_node->children[i]) {
						child = _get_min_node(cur_node->children[i]);
						break;
					}
				}

				if (child) {
					cur_node = child;
					continue;
				}

				// The node has no child: detach it from its parent, so that the parent's
				// children scan never reaches the freed node, then delete it.
				Node *parent = cur_node->p;
				if (parent)
					parent->children[cur_node->offset] = nullptr;

				_delete_single_node(cur_node);
				cur_node = parent;
			}

			_root = nullptr;
			_size = 0;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(K key, V &&value) {
			return _insert(key, std::move(value));
		}

		inline Option<V> remove(const K &index) noexcept {
			Node *node = _root;
			Height height = _height;

			while (height > R) {
				K offset = (index >> (height - R)) & MASK;
				if (!node->children[offset])
					return NULLOPT;
				node = node->children[offset];
				height -= R;
			}

			K offset = index & MASK;
			if (!node->radix_value.has_value(offset))
				return NULLOPT;

			V removed = std::move(node->radix_value.value(offset));
			node->radix_value.move(offset);
			--node->num_used_children;

			while (node->num_used_children == 0 && node != _root) {
				Node *parent = node->p;
				parent->children[node->offset] = nullptr;
				--parent->num_used_children;
				_delete_single_node(node);
				node = parent;
			}

			if (node == _root && (!node->num_used_children)) {
				_delete_single_node(_root);
				_root = nullptr;
				_height = 0;
			}

			--_size;

			return std::move(removed);
		}

		PEFF_FORCEINLINE const V &at(K key) const {
			auto result = _lookup(key);
			assert(result.has_value());
			return result->first->radix_value.value(result->second);
		}

		PEFF_FORCEINLINE V &at(K key) {
			auto result = _lookup(key);
			assert(result.has_value());
			return result->first->radix_value.value(result->second);
		}

		PEFF_FORCEINLINE peff::Option<const V &> try_at(K key) const {
			auto result = _lookup(key);
			if (!result.has_value())
				return peff::NULLOPT;
			return result->first->radix_value.value(result->second);
		}

		PEFF_FORCEINLINE peff::Option<V &> try_at(K key) {
			auto result = _lookup(key);
			if (!result.has_value())
				return peff::NULLOPT;
			return result->first->radix_value.value(result->second);
		}

		PEFF_FORCEINLINE bool contains(K key) const noexcept {
			return _lookup(key).has_value();
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _size;
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return !_size;
		}

		static Node *get_next_node(PEFF_NONNULL const Node *leaf) {
			const Node *cur = leaf;
			while (cur->p) {
				size_t off = cur->offset;
				const Node *parent = cur->p;
				for (size_t i = off + 1; i < N; ++i) {
					if (parent->children[i])
						return _get_min_node(parent->children[i]);
				}
				cur = parent;
			}
			return nullptr;
		}

		static Node *get_prev_node(PEFF_NONNULL const Node *leaf) {
			const Node *cur = leaf;
			while (cur->p) {
				size_t off = cur->offset;
				const Node *parent = cur->p;
				for (size_t i = off; i-- > 0;) {
					if (parent->children[i])
						return _get_max_node(parent->children[i]);
				}
				cur = parent;
			}
			return nullptr;
		}

		struct Iterator {
			Node *node;
			ThisType *tree;
			IteratorDirection direction;
			size_t slot_index;

			PEFF_FORCEINLINE Iterator(
				PEFF_NONNULL Node *node,
				PEFF_NONNULL ThisType *tree,
				IteratorDirection direction,
				size_t slot_index)
				: node(node), tree(tree), direction(direction), slot_index(slot_index) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;
				slot_index = it.slot_index;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				PEFF_ASSERT(direction == rhs.direction, "Incompatible iterator direction");
				node = rhs.node;
				tree = rhs.tree;
				slot_index = rhs.slot_index;
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
					for (size_t i = slot_index + 1; i < N; ++i) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}

					node = ThisType::get_next_node(node);
					if (!node) {
						slot_index = 0;
						return *this;
					}

					for (size_t i = 0; i < N; ++i) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}

					throw std::logic_error("Corrupted tree: empty leaf in forward traversal");
				} else {
					for (size_t i = slot_index; i-- > 0;) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					node = ThisType::get_prev_node(node);
					if (!node) {
						slot_index = 0;
						return *this;
					}
					for (size_t i = N; i-- > 0;) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					throw std::logic_error("Corrupted tree: empty leaf in backward traversal");
				}
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
					if (node == tree->_get_min_node(tree->_root) && slot_index == [&]() {
							for (size_t i = 0; i < N; ++i) {
								if (node->radix_value.has_value(i))
									return i;
							}
							return size_t(0);
						}())
						throw std::logic_error("Decreasing the begin iterator");

					for (size_t i = slot_index; i-- > 0;) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					if (!node)
						node = (Node *)tree->_get_max_node(tree->_root);
					else
						node = ThisType::get_prev_node(node);
					if (!node) throw std::logic_error("Decreasing invalid");
					for (size_t i = N; i-- > 0;) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					throw std::logic_error("Corrupted tree");
				} else {
					if (node == tree->_get_max_node(tree->_root) && slot_index == [&]() {
							for (size_t i = N; i-- > 0;) {
								if (node->radix_value.has_value(i))
									return i;
							}
							return size_t(N - 1);
						}())
						throw std::logic_error("Decreasing the begin iterator");

					for (size_t i = slot_index + 1; i < N; ++i) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					if (!node)
						node = (Node *)tree->_get_min_node(tree->_root);
					else
						node = ThisType::get_next_node(node);
					if (!node) throw std::logic_error("Decreasing invalid");
					for (size_t i = 0; i < N; ++i) {
						if (node->radix_value.has_value(i)) {
							slot_index = i;
							return *this;
						}
					}
					throw std::logic_error("Corrupted tree");
				}
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

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				if (tree != it.tree)
					throw std::logic_error("Cannot compare iterators from different trees");
				return node == it.node && slot_index == it.slot_index;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &it) const {
				return !(*this == it);
			}

			PEFF_FORCEINLINE V &operator*() {
				if (!node)
					throw std::logic_error("Dereferencing the end iterator");
				return node->radix_value.value(slot_index);
			}

			PEFF_FORCEINLINE const V &operator*() const {
				if (!node)
					throw std::logic_error("Dereferencing the end iterator");
				return node->radix_value.value(slot_index);
			}

			PEFF_FORCEINLINE V *operator->() {
				if (!node)
					throw std::logic_error("Dereferencing the end iterator");
				return &node->radix_value.value(slot_index);
			}

			PEFF_FORCEINLINE const V *operator->() const {
				if (!node)
					throw std::logic_error("Dereferencing the end iterator");
				return &node->radix_value.value(slot_index);
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			Node *node = _get_min_node(_root);
			if (!node)
				return end();
			size_t slot = 0;
			for (; slot < N; ++slot) {
				if (node->radix_value.has_value(slot))
					break;
			}
			return Iterator(node, this, IteratorDirection::Forward, slot);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward, 0);
		}
		PEFF_FORCEINLINE Iterator rbegin() {
			Node *node = _root ? _get_max_node(_root) : nullptr;
			if (!node)
				return rend();
			size_t slot = N - 1;
			while (slot && !node->radix_value.has_value(slot))
				--slot;
			return Iterator(node, this, IteratorDirection::Reversed, node->radix_value.has_value(slot) ? slot : 0);
		}

		PEFF_FORCEINLINE Iterator rend() {
			return Iterator(nullptr, this, IteratorDirection::Reversed, 0);
		}

		struct ConstIterator {
			Iterator _iterator;

			PEFF_FORCEINLINE ConstIterator(Iterator &&iterator) : _iterator(iterator) {}
			ConstIterator(const ConstIterator &it) = default;
			PEFF_FORCEINLINE ConstIterator(ConstIterator &&it) : _iterator(std::move(it._iterator)) {}
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
				ConstIterator it = *this;
				return ++it;
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
				ConstIterator it = *this;
				return --it;
			}

			PEFF_FORCEINLINE bool operator==(const ConstIterator &it) const {
				return _iterator == it._iterator;
			}
			PEFF_FORCEINLINE bool operator!=(const ConstIterator &it) const {
				return _iterator != it._iterator;
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

		PEFF_FORCEINLINE Iterator find(K key) noexcept {
			auto result = _lookup(key);
			if (!result.has_value())
				return end();
			return Iterator(result->first, this, IteratorDirection::Forward, result->second);
		}

		PEFF_FORCEINLINE ConstIterator find(K key) const noexcept {
			return ConstIterator(find(key));
		}
	};
}

#endif
