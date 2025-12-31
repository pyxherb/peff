#ifndef _PEFF_CONTAINERS_RADIX_TREE_H_
#define _PEFF_CONTAINERS_RADIX_TREE_H_

#include "basedefs.h"
#include "bitset.h"
#include <peff/utils/option.h>
#include <peff/utils/misc.h>
#include <peff/utils/bitops.h>
#include <peff/base/alloc.h>
#include <peff/base/scope_guard.h>

namespace peff {
	template <typename K, typename V>
	class RadixTree final {
	public:
		static_assert(std::is_integral_v<K>, "The key must be integral type");
		using ThisType = RadixTree<K, V>;
		
		using Height = size_t;

		struct Node {
			size_t nUsedChildren = 0;
			Node *p = nullptr;
			size_t offset = 0;
			Node *children[2] = {};
			Option<V> radixValue;
			Height height = 0;

			PEFF_FORCEINLINE Node() {
			}
			PEFF_FORCEINLINE Node(K &&value) : radixValue(std::move(value)) {
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

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocSingleNode(V &&value) {
			Node *node = (Node *)allocAndConstruct<Node>(_allocator.get(), alignof(Node), std::move(value));
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

		[[nodiscard]] inline bool _growNode(K index, Node *newNode) {
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

				Node *&slot = node->children[offset];
				if (!slot) {
					slot = _allocSingleNode();
					if (!slot)
						return false;
					++_nNodes;
					slot->offset = offset;
					slot->p = node;
				}
				node = slot;

				insertionFlags.setBit(i);
				++i;
			}

			sg.release();

			Node *leaf = node;
			offset = index & 1;
			if (leaf->children[offset])
				// The key exists.
				std::terminate();
			leaf->children[offset] = newNode;
			++leaf->nUsedChildren;
			return true;
		}

		[[nodiscard]] inline bool _growHeightThenGrowNode(K index, Height height, Node *node) {
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

			if (!_growNode(index, node))
				return false;

			restoreGuard.release();

			return true;
		}

		[[nodiscard]] inline bool _insert(K index, Node *newNode) {
			Node **pNode = &_root;
			Height height = (Height)((sizeof(K) * 8 - 1 - countLeadingZero(index)) + 1);

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
				if (!_growNode(index, newNode))
					return false;
				restorePropertiesGuard.release();
				deleteNodeGuard.release();
				return true;
			}

			if (height > _height)
				return _growHeightThenGrowNode(index, height, newNode);

			return _growNode(index, newNode);
		}

		[[nodiscard]] inline Node *_lookup(K index) const {
			if (!_root)
				return nullptr;
			Node *node = _root;
			Height height = _height;
			Height shift = height - 1;
			K offset;

			while (height) {
				offset = ((index >> shift) & 1);
				node = node->children[offset];
				if (!node)
					return nullptr;
				--shift;
				--height;
			}
		}

		[[nodiscard]] inline Node* _getMinNode(Node* node) const {
			K offset;
			Height height;

			assert(node);

			height = node->height;
			while (height) {
				if (!node)
					return nullptr;
				offset = 0;
				while ((offset < 2) && (!node->children[offset]))
					++offset;
				node = node->children[offset];
				--height;
			}
			return node;
		}

		[[nodiscard]] inline Node* _lookupUpperBound(K index) const {
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
				if (curNode->children[0]) {
					curNode = (Node *)_getMinNode(curNode->children[0]);
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

		[[nodiscard]] PEFF_FORCEINLINE bool insert(K key, V&& value) {
			Node *newNode = _allocSingleNode(std::move(value));

			if (!newNode)
				return false;

			peff::ScopeGuard releaseNodeGuard([this, newNode]() noexcept {
				_deleteSingleNode(newNode);
			});

			if (!_insert(key, newNode))
				return false;

			releaseNodeGuard.release();

			return true;
		}

		PEFF_FORCEINLINE size_t size() {
			return _nNodes;
		}
	};
}

#endif
