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

namespace peff {
	template <typename K, typename V>
	class RadixTree final {
	public:
		static_assert(std::is_integral_v<K>, "The key must be integral type");
		using ThisType = RadixTree<K, V>;

		constexpr static uintmax_t HEIGHT_MAX = std::numeric_limits<K>::digits;
		using Height = typename ::peff::AutoSizeUInteger<HEIGHT_MAX>::type;

		struct Node {
			Height height = 0;
			uint8_t nUsedChildren = 0;
			uint8_t offset = 0;
			Node *p = nullptr;
			Node *children[2] = {};
			Option<V> radixValue[2];

			PEFF_FORCEINLINE Node() {
			}
		};

	private:
		peff::RcObjectPtr<peff::Alloc> _allocator;
		Height _height;
		size_t _nNodes;
		Node *_root;

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode() {
			Node *node = allocAndConstruct<Node>(_allocator.get(), alignof(Node));
			if (!node)
				return nullptr;

			return node;
		}

		PEFF_FORCEINLINE void _deleteSingleNode(Node *node) {
			destroyAndRelease<Node>(_allocator.get(), node, alignof(Node));
		}

		[[nodiscard]] inline bool _growHeight(Height height) {
			Node *originalRoot = _root, *firstNode = nullptr;
			Height originalHeight = _height;

			peff::ScopeGuard restoreGuard([this, &firstNode, originalRoot, originalHeight]() noexcept {
				for (Node *i = firstNode; i; i = i->p) {
					_deleteSingleNode(i);
				}

				if (originalRoot)
					originalRoot->p = nullptr;

				_root = originalRoot;
				_height = originalHeight;
			});

			while (_height < height) {
				++_height;

				Node *newNode = _allocSingleNode();
				if (!newNode)
					return false;

				if (!firstNode)
					firstNode = newNode;

				newNode->height = _height;
				newNode->offset = 0;

				_root->p = newNode;
				newNode->children[0] = _root;
				++newNode->nUsedChildren;
				_root = newNode;
			}

			restoreGuard.release();

			return true;
		}

		[[nodiscard]] inline bool _growNode(K index, V &&data) {
			Node *node = _root;
			Height height = _height, shift;
			K offset;

			size_t i = 0;
			BitSet<sizeof(K) * 8 - 1> insertionFlags;

			peff::ScopeGuard sg([this, &insertionFlags, &i, &node]() noexcept {
				while (i && node) {
					--i;
					if (insertionFlags.getBit(i)) {
						Node *parent = node->p;
						K offset = node->offset;
						--_nNodes;
						_deleteSingleNode(node);
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

				if ((!node->children[offset]) && (!node->radixValue[offset].hasValue())) {
					Node *newNode = _allocSingleNode();
					if (!newNode)
						return false;
					newNode->height = height;
					++node->nUsedChildren;
					newNode->offset = offset;
					newNode->p = node;
					node->children[offset] = newNode;

					node = newNode;
					insertionFlags.setBit(i);
				} else {
					assert(!node->radixValue[offset]);
					node = node->children[offset];
				}
				++i;
			}

			sg.release();

			Node *leaf = node;
			offset = index & 1;
			if (leaf->children[offset] || leaf->radixValue[offset])
				// The key exists.
				std::terminate();
			leaf->radixValue[offset] = std::move(data);
			return true;
		}

		[[nodiscard]] inline bool _growHeightThenGrowNode(K index, Height height, V &&data) {
			Node *originalRoot = _root, *firstNode = nullptr;
			Height originalHeight = _height;

			peff::ScopeGuard restoreGuard([this, &firstNode, originalRoot, originalHeight]() noexcept {
				for (Node *i = firstNode; i; i = i->p) {
					_deleteSingleNode(i);
				}

				if (originalRoot)
					originalRoot->p = nullptr;

				_root = originalRoot;
				_height = originalHeight;
			});

			while (_height < height) {
				++_height;

				Node *newNode = _allocSingleNode();
				if (!newNode)
					return false;

				if (!firstNode)
					firstNode = newNode;

				newNode->height = _height;
				newNode->offset = 0;

				_root->p = newNode;
				newNode->children[0] = _root;
				++newNode->nUsedChildren;
				_root = newNode;
			}

			if (!_growNode(index, std::move(data)))
				return false;

			restoreGuard.release();

			return true;
		}

		[[nodiscard]] inline bool _insert(K index, V &&data) {
			Node **pNode = &_root;
			Height height = index ? (Height)((sizeof(K) * 8 - 1 - countLeadingZero(index)) + 1) : 1;

			if (!_root) {
				Node *node = _allocSingleNode();
				if (!node)
					return false;
				peff::ScopeGuard deleteNodeGuard([this, node]() noexcept {
					_deleteSingleNode(node);
				});
				node->height = height;
				_height = height;

				Height originalHeight = _height;
				Node *originalRoot = _root;

				peff::ScopeGuard restorePropertiesGuard([this, originalRoot, originalHeight]() noexcept {
					_root = originalRoot;
					_height = originalHeight;
				});

				_root = node;
				if (!_growNode(index, std::move(data)))
					return false;
				restorePropertiesGuard.release();
				deleteNodeGuard.release();
				return true;
			}

			if (height > _height)
				return _growHeightThenGrowNode(index, height, std::move(data));

			return _growNode(index, std::move(data));
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
					if (!node->radixValue[offset].hasValue())
						std::terminate();
					return *node->radixValue[offset];
				}
				node = node->children[offset];
				--shift;
				--height;
			}
		}

		[[nodiscard]] static inline Node *_getMinNode(Node *node) {
			assert(node);

			while (node->children[0] || node->children[1]) {
				node = node->children[0] ? node->children[0] : node->children[1];
			}

			return node;
		}

		[[nodiscard]] static inline Node *_getMaxNode(Node *node) {
			assert(node);

			while (node->children[0] || node->children[1]) {
				node = node->children[1] ? node->children[1] : node->children[0];
			}

			return node;
		}

		[[nodiscard]] inline Node *_lookupUpperBound(K index) const {
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
						return _getMinNode(node->children[offset + 1]);
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
					return _getMinNode(node->children[off + 1]);
				}
			}

			return nullptr;
		}

		[[nodiscard]] Node *_deleteTree(K index) {
			Node *node = _root;
			Height height = _height;
			Height shift = height - 1;
			K offset;

			if (height < ((sizeof(K) * 8 - 1 - countLeadingZero(index)) + 1))
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
			--node->nUsedChildren;

			while ((!node->nUsedChildren) && (node != _root)) {
				Node *p = node->p;
				K off = node->offset;
				_deleteSingleNode(node);
				p->children[off] = nullptr;
				--p->nUsedChildren;
				node = p;
			}

			if (_root && (!_root->nUsedChildren)) {
				_deleteSingleNode(node);
				_root = nullptr;
				_height = 0;
			}

			return item;
		}

	public:
		PEFF_FORCEINLINE RadixTree(peff::Alloc *allocator) : _allocator(allocator), _height(0), _nNodes(0), _root(nullptr) {}
		PEFF_FORCEINLINE ~RadixTree() {
			Node *curNode = (Node *)_getMinNode(_root);
			Node *parent = (Node *)_root->p;
			bool walkedRootNode = false;

			while (curNode != parent) {
				if (curNode->children[1]) {
					curNode = (Node *)_getMinNode(curNode->children[1]);
				} else {
					Node *nodeToDelete = curNode;

					while (curNode->p && (curNode == curNode->p->children[1])) {
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
			return _nNodes;
		}

		PEFF_FORCEINLINE static Node *getNextNode(const Node *node) noexcept {
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

		PEFF_FORCEINLINE static Node *getPrevNode(const Node *node) noexcept {
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
			bool isRight;

			PEFF_FORCEINLINE Iterator(
				Node *node,
				ThisType *tree,
				IteratorDirection direction,
				bool isRight)
				: node(node),
				  tree(tree),
				  direction(direction),
				  isRight(isRight) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				tree = it.tree;
				direction = it.direction;
				isRight = it.isRight;

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
					if (!isRight) {
						if (node->radixValue[1]) {
							isRight = true;
							return *this;
						}
					}
					isRight = false;
					node = ThisType::getNextNode(node);
				} else {
					if (isRight) {
						if (node->radixValue[0]) {
							isRight = false;
							return *this;
						}
					}
					isRight = true;
					node = ThisType::getPrevNode(node);
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
					if (node == tree->_getMinNode(tree->_root))
						throw std::logic_error("Dereasing the begin iterator");

					if (!isRight) {
						if (node->radixValue[0]) {
							isRight = true;
							return *this;
						}
					}
					isRight = false;

					if (!node)
						node = (Node *)tree->_getMaxNode(tree->_root);
					else
						node = ThisType::getPrevNode(node);
				} else {
					if (node == tree->_getMaxNode(tree->_root))
						throw std::logic_error("Dereasing the begin iterator");

					if (isRight) {
						if (node->radixValue[0]) {
							isRight = false;
							return *this;
						}
					}
					isRight = true;

					if (!node)
						node = (Node *)tree->_getMinNode(tree->_root);
					else
						node = ThisType::getNextNode(node);
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
				return isRight == it.isRight;
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
				return isRight != it.isRight;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE V &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return isRight ? *node->radixValue[1] : *node->radixValue[0];
			}

			PEFF_FORCEINLINE V &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return isRight ? *node->radixValue[1] : *node->radixValue[0];
			}

			PEFF_FORCEINLINE V *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return isRight ? &*node->radixValue[1] : &*node->radixValue[0];
			}

			PEFF_FORCEINLINE V *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return isRight ? &*node->radixValue[1] : &*node->radixValue[0];
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			Node *node = _getMinNode(_root);
			bool isRight;
			if (node)
				isRight = node->radixValue[0] ? false : true;
			else
				isRight = false;
			return Iterator((Node *)node, this, IteratorDirection::Forward, isRight);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward, false);
		}
		PEFF_FORCEINLINE Iterator beginReversed() {
			Node *node = _getMaxNode(_root);
			bool isRight;
			if (node)
				isRight = node->radixValue[1] ? true : false;
			else
				isRight = false;
			return Iterator((Node *)node, this, IteratorDirection::Reversed, isRight);
		}
		PEFF_FORCEINLINE Iterator endReversed() {
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
	};
}

#endif
