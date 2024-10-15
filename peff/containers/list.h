#ifndef _PEFF_CONTAINERS_LIST_H_
#define _PEFF_CONTAINERS_LIST_H_

#include "basedefs.h"
#include <memory_resource>
#include <cassert>
#include <functional>
#include <peff/base/allocator.h>
#include <peff/utils/scope_guard.h>
#include <peff/utils/misc.h>

namespace peff {
	template <typename T, typename Allocator = StdAlloc>
	class List {
	public:
		struct Node {
			Node *prev = nullptr, *next = nullptr;
			T data;

			Node() = default;
			PEFF_FORCEINLINE Node(T &&data) : data(data) {
			}
		};

	private:
		using ThisType = List<T, Allocator>;

		Node *_first = nullptr, *_last = nullptr;
		size_t _length = 0;
		Allocator _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocNode() {
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

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocNode(const T &data) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard(
				[this, node]() {
					_allocator.release(node);
				});

			new (node) Node();
			if (!peff::copy(node->data, data)) {
				return false;
			}
			scopeGuard.release();

			return node;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *_allocNode(T &&data) {
			Node *node = (Node *)_allocator.alloc(sizeof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scopeGuard([this, node]() {
				_allocator.release(node);
			});
			new (node) Node(std::move(data));
			scopeGuard.release();

			return node;
		}

		PEFF_FORCEINLINE void _deleteNode(Node *node) {
			std::destroy_at<Node>(node);

			_allocator.release(node);
		}

		PEFF_FORCEINLINE void _prepend(Node *dest, Node *node) noexcept {
			if (dest) {
				if (dest->prev)
					dest->prev->next = node;
				dest->prev = node;

				if (dest == _first)
					_first = node;
			}
			node->next = dest;

			if (!_first)
				_first = node;
			if (!_last)
				_last = node;

			++_length;
		}

		PEFF_FORCEINLINE void _append(Node *dest, Node *node) noexcept {
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
			if (!_last)
				_last = node;

			++_length;
		}

		PEFF_FORCEINLINE void _remove(Node *dest) {
			if (dest == _first) {
				_first = dest->next;
			} else if (dest == _last) {
				_last = dest->prev;
			}

			if (dest->prev)
				dest->prev->next = dest->next;
			if (dest->next)
				dest->next->prev = dest->prev;

			--_length;
		}

	public:
		PEFF_FORCEINLINE List() = default;
		List(const ThisType &other) = delete;
		PEFF_FORCEINLINE List(ThisType &&other) {
			_first = other._first;
			_last = other._last;
			_length = other._length;
			_allocator = std::move(other._allocator);

			other._first = nullptr;
			other._last = nullptr;
			other._length = 0;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) {
			clear();
			new (this) List(std::move(other));
		}
		PEFF_FORCEINLINE ThisType &operator=(const ThisType &other) = delete;

		[[nodiscard]] PEFF_FORCEINLINE bool copy(List &dest) const {
			dest._first = _first;
			dest._last = _last;
			dest._length = _length;

			if (!peff::copy(dest._allocator, _allocator))
				return false;

			for (Node *i = _first; i; i = i->next) {
				Node *newNode = dest._allocNode(i->data);
				if (!newNode) {
					dest.clear();
					return false;
				}
				dest.pushBack(newNode);
			}

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool copyAssign(List &dest) const {
			dest.clear();

			dest._first = _first;
			dest._last = _last;
			dest._length = _length;

			if (!peff::copyAssign(dest._allocator, _allocator))
				return false;

			for (Node *i = _first; i; i = i->next) {
				Node *newNode = dest._allocNode(i->data);
				if (!newNode) {
					dest.clear();
					return false;
				}
				dest.pushBack(newNode);
			}

			return true;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insertFront(Node *node, const T &data) {
			assert(node);

			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;

			_prepend(node, newNode);

			return newNode;
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *insertBack(Node *node, const T &data) {
			assert(node);

			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;

			_append(node, newNode);

			return newNode;
		}

		PEFF_FORCEINLINE void pushFront(Node *node) noexcept {
			_prepend(_first, node);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *pushFront(const T &data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_prepend(_first, newNode);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *pushFront(T &&data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_prepend(_first, newNode);
			return newNode;
		}

		PEFF_FORCEINLINE void pushBack(Node *node) noexcept {
			_append(_last, node);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *pushBack(const T &data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_append(_last, newNode);
		}

		[[nodiscard]] PEFF_FORCEINLINE Node *pushBack(T &&data) {
			Node *newNode = _allocNode(data);
			if (!newNode)
				return nullptr;
			_append(_last, newNode);
			return newNode;
		}

		PEFF_FORCEINLINE void popFront() {
			remove(_first);
		}

		PEFF_FORCEINLINE void popBack() {
			remove(_last);
		}

		PEFF_FORCEINLINE void remove(Node *node) {
			_remove(node);
			_deleteNode(node);
		}

		PEFF_FORCEINLINE void detach(Node *node) {
			_remove(node);
		}

		PEFF_FORCEINLINE static Node *next(Node *curNode, size_t index) {
			while (index) {
				assert(curNode);
				curNode = curNode->next;
				--index;
			}

			return curNode;
		}

		PEFF_FORCEINLINE static Node *prev(Node *curNode, size_t index) {
			while (index) {
				assert(curNode);
				curNode = curNode->prev;
				--index;
			}

			return curNode;
		}

		PEFF_FORCEINLINE Node *firstNode() {
			return _first;
		}

		PEFF_FORCEINLINE const Node *firstNode() const {
			return _first;
		}

		PEFF_FORCEINLINE Node *lastNode() {
			return _last;
		}

		PEFF_FORCEINLINE const Node *lastNode() const {
			return _last;
		}

		PEFF_FORCEINLINE T &front() {
			return _first->data;
		}

		PEFF_FORCEINLINE const T &front() const {
			return _first->data;
		}

		PEFF_FORCEINLINE T &back() {
			return _last->data;
		}

		PEFF_FORCEINLINE const T &back() const {
			return _last->data;
		}

		PEFF_FORCEINLINE void clear() {
			for (Node *i = _first; i;) {
				Node *nextNode = i->next;

				_deleteNode(i);

				i = nextNode;
			}
		}

		PEFF_FORCEINLINE size_t getSize() {
			return _length;
		}
	};
}

#endif
