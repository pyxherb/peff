#ifndef __PHUL_LIST_H__
#define __PHUL_LIST_H__

#include "basedefs.h"
#include <memory_resource>
#include <cassert>
#include <functional>
#include <phul/base/allocator.h>
#include <phul/utils/scope_guard.h>

namespace phul {
	template <typename T, typename Allocator = StdAlloc>
	class List {
	public:
		struct Node {
			Node *prev = nullptr, *next = nullptr;
			T data;

			Node() = default;
			inline Node(const T &data) : data(data) {}
			inline Node(T &&data) : data(data) {}
		};

	private:
		using ThisType = List<T, Allocator>;

		Node *_first = nullptr, *_last = nullptr;
		size_t _length = 0;
		Allocator _allocator;

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocNode() {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard([this, node]() {
				_allocator.release(node);
			});

			new (node) Node();
			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocNode(const T &data) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator.release(node);
				});

			new (node) Node(data);
			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *_allocNode(T &&data) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard([this, node]() {
				_allocator.release(node);
			});
			new (node) Node(data);
			scopeGuard.release();

			return node;
		}

		PHUL_FORCEINLINE void _deleteNode(Node *node) {
			std::destroy_at<Node>(node);

			_allocator.release(node);
		}

		PHUL_FORCEINLINE void _prepend(Node *dest, Node *node) noexcept {
			if (dest) {
				if (dest->prev)
					dest->prev->next = node;
				dest->prev = node;

				if (dest == _first)
					_first = node;
			}
			node->next = dest;

			if (!_last)
				_last = node;
		}

		PHUL_FORCEINLINE void _append(Node *dest, Node *node) noexcept {
			if (dest) {
				if (dest->next)
					dest->next->prev = node;
				dest->next = node;

				if (dest == _last)
					_last = node;
			}
			node->prev = dest;

			if (!_first)
				_first = node;
		}

		PHUL_FORCEINLINE void _remove(Node *dest) {
			if (dest == _first) {
				_first = dest->next;
			} else if (dest == _last) {
				_first = dest->prev;
			}

			if (dest->prev)
				dest->prev->next = dest->next;
			if (dest->next)
				dest->next->prev = dest->prev;
		}

	public:
		List(const ThisType &other) = delete;
		PHUL_FORCEINLINE List(ThisType &&other) {
			_first = other._first;
			_last = other._last;
			_length = other._length;
			_allocator = std::move(other._allocator);

			other._first = nullptr;
			other._last = nullptr;
			other._length = 0;
		}
		PHUL_FORCEINLINE ThisType &operator=(ThisType &&other) {
			clear();
			new (this) List(std::move(other));
		}
		PHUL_FORCEINLINE ThisType &operator=(const ThisType &other) = delete;

		[[nodiscard]] PHUL_FORCEINLINE bool copy(List &dest) const {
			if constexpr (IsCopyable<Allocator>::value) {
				dest.clear();

				dest._first = _first;
				dest._last = _last;
				dest._length = _length;
				dest._allocator = _allocator;

				for (Node *i = _first; i; i = i->next) {
					Node *newNode = dest._allocNode(i->data);
					if (!newNode) {
						dest.clear();
						return false;
					}
					dest.pushBack(newNode);
				}

				return true;
			} else {
				throw std::runtime_error("The type is not copyable");
			}
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *insertFront(Node *node, const T &data) {
			assert(node);

			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;

			_prepend(node, newNode);

			return newNode;
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *insertBack(Node *node, const T &data) {
			assert(node);

			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;

			_append(node, newNode);

			return newNode;
		}

		PHUL_FORCEINLINE void pushFront(Node *node) noexcept {
			_prepend(_first, node);
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *pushFront(const T &data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_prepend(_first, newNode);
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *pushFront(T &&data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_prepend(_first, newNode);
			return newNode;
		}

		PHUL_FORCEINLINE void pushBack(Node *node) noexcept {
			_append(_last, node);
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *pushBack(const T &data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_append(_last, newNode);
		}

		[[nodiscard]] PHUL_FORCEINLINE Node *pushBack(T &&data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_append(_last, newNode);
			return newNode;
		}

		PHUL_FORCEINLINE void popFront() {
			remove(_first);
		}

		PHUL_FORCEINLINE void popBack() {
			remove(_last);
		}

		PHUL_FORCEINLINE void remove(Node *node) {
			_remove(node);
			_deleteNode(node);
		}

		PHUL_FORCEINLINE void detach(Node *node) {
			_remove(node);
		}

		PHUL_FORCEINLINE static Node *next(Node *curNode, size_t index) {
			while (index) {
				assert(curNode);
				curNode = curNode->next;
				--index;
			}

			return curNode;
		}

		PHUL_FORCEINLINE static Node *prev(Node *curNode, size_t index) {
			while (index) {
				assert(curNode);
				curNode = curNode->prev;
				--index;
			}

			return curNode;
		}

		PHUL_FORCEINLINE Node *firstNode() {
			return _first;
		}

		PHUL_FORCEINLINE const Node *firstNode() const {
			return _first;
		}

		PHUL_FORCEINLINE Node *lastNode() {
			return _last;
		}

		PHUL_FORCEINLINE const Node *lastNode() const {
			return _last;
		}

		PHUL_FORCEINLINE T &front() {
			return _first->data;
		}

		PHUL_FORCEINLINE const T &front() const {
			return _first->data;
		}

		PHUL_FORCEINLINE T &back() {
			return _last->data;
		}

		PHUL_FORCEINLINE const T &back() const {
			return _last->data;
		}

		PHUL_FORCEINLINE void clear() {
			for (Node *i = _first; i;) {
				Node *nextNode = i->next;

				_deleteNode(i);

				i = nextNode;
			}
		}
	};
}

#endif
