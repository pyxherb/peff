#ifndef _PEFF_CONTAINERS_LIST_H_
#define _PEFF_CONTAINERS_LIST_H_

#include "basedefs.h"
#include "misc.h"
#include <memory_resource>
#include <cassert>
#include <functional>
#include <peff/base/alloc.h>
#include <peff/base/scope_guard.h>
#include <peff/base/misc.h>
#include <stdexcept>

namespace peff {
	template <typename T>
	class List {
	public:
		struct Node {
			Node *prev = nullptr, *next = nullptr;
			T data;

			PEFF_FORCEINLINE Node(T &&data) : data(std::move(data)) {
			}
		};

		using NodeHandle = Node *;

		static PEFF_FORCEINLINE NodeHandle null_node_handle() {
			return nullptr;
		}

	private:
		using ThisType = List<T>;

		Node *_first = nullptr, *_last = nullptr;
		size_t _length = 0;
		RcObjectPtr<Alloc> _allocator;

		[[nodiscard]] PEFF_FORCEINLINE Node *_alloc_node(T &&data) {
			Node *node = (Node *)_allocator->alloc(sizeof(Node), alignof(Node));
			if (!node)
				return nullptr;

			ScopeGuard scope_guard([this, node]() noexcept {
				_allocator->release(node, sizeof(Node), alignof(Node));
			});
			peff::construct_at<Node>(node, std::move(data));
			scope_guard.release();

			return node;
		}

		PEFF_FORCEINLINE void _delete_node(Node *node) {
			std::destroy_at<Node>(node);

			_allocator->release(node, sizeof(Node), alignof(Node));
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
			assert(dest);

			if (dest == _first) {
				_first = dest->next;
			}
			if (dest == _last) {
				_last = dest->prev;
			}

			if (dest->prev)
				dest->prev->next = dest->next;
			if (dest->next)
				dest->next->prev = dest->prev;

			--_length;
		}

	public:
		PEFF_FORCEINLINE List(Alloc *allocator) : _allocator(allocator) {}
		List(const ThisType &other) = delete;
		PEFF_FORCEINLINE List(ThisType &&other) : _first(other._first), _last(other._last), _length(other._length), _allocator(std::move(other._allocator)) {
			other._first = nullptr;
			other._last = nullptr;
			other._length = 0;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) {
			clear();
			peff::construct_at<List>(this, std::move(other));
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(const ThisType &other) = delete;
		PEFF_FORCEINLINE ~List() {
			for (Node *i = _first; i != nullptr;) {
				Node *next = i->next;

				_delete_node(i);

				i = next;
			}
		}

		[[nodiscard]] PEFF_FORCEINLINE NodeHandle insert_front(NodeHandle node, NodeHandle new_node) {
			assert(node);

			_prepend(node, new_node);

			return new_node;
		}

		[[nodiscard]] PEFF_FORCEINLINE NodeHandle insert_back(NodeHandle node, NodeHandle new_node) {
			assert(node);

			_append(node, new_node);

			return new_node;
		}

		PEFF_FORCEINLINE void push_front(NodeHandle node) noexcept {
			_prepend(_first, node);
		}

		[[nodiscard]] PEFF_FORCEINLINE NodeHandle push_front(T &&data) {
			Node *new_node = _alloc_node(std::move(data));
			if (!new_node)
				return nullptr;
			_prepend(_first, new_node);
			return new_node;
		}

		PEFF_FORCEINLINE void push_back(NodeHandle node) noexcept {
			_append(_last, node);
		}

		[[nodiscard]] PEFF_FORCEINLINE NodeHandle push_back(T &&data) {
			Node *new_node = _alloc_node(std::move(data));
			if (!new_node)
				return nullptr;
			_append(_last, new_node);
			return new_node;
		}

		PEFF_FORCEINLINE void pop_front() {
			remove(_first);
		}

		PEFF_FORCEINLINE void pop_back() {
			remove(_last);
		}

		PEFF_FORCEINLINE void remove(NodeHandle node) {
			_remove(node);
			_delete_node(node);
		}

		PEFF_FORCEINLINE void detach(NodeHandle node) {
			_remove(node);
		}

		PEFF_FORCEINLINE static Node *next(NodeHandle cur_node, size_t index) {
			while (index) {
				assert(cur_node);
				cur_node = cur_node->next;
				--index;
			}

			return cur_node;
		}

		PEFF_FORCEINLINE static Node *prev(NodeHandle cur_node, size_t index) {
			while (index) {
				assert(cur_node);
				cur_node = cur_node->prev;
				--index;
			}

			return cur_node;
		}

		PEFF_FORCEINLINE void delete_node(NodeHandle node) {
			_delete_node(node);
		}

		PEFF_FORCEINLINE NodeHandle first_node() {
			return _first;
		}

		PEFF_FORCEINLINE const NodeHandle first_node() const {
			return _first;
		}

		PEFF_FORCEINLINE NodeHandle last_node() {
			return _last;
		}

		PEFF_FORCEINLINE const NodeHandle last_node() const {
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
				Node *next_node = i->next;

				_delete_node(i);

				i = next_node;
			}
			_length = 0;
			_first = nullptr;
			_last = nullptr;
		}

		PEFF_FORCEINLINE size_t size() const {
			return _length;
		}
		struct Iterator {
			Node *node;
			ThisType *list;
			IteratorDirection direction;

			PEFF_FORCEINLINE Iterator(
				Node *node,
				ThisType *list,
				IteratorDirection direction)
				: node(node),
				  list(list),
				  direction(direction) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				node = it.node;
				list = it.list;
				direction = it.direction;

				it.node = nullptr;
				it.list = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				list = rhs.list;
				return *this;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				list = rhs.list;
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
					node = node->next;
				} else {
					node = node->prev;
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++(*this);
				return it;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (!(node = node->prev))
						throw std::logic_error("Dereasing the begin iterator");
				} else {
					if (!(node = node->next))
						throw std::logic_error("Dereasing the begin iterator");
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				if (list != it.list)
					throw std::logic_error("Cannot compare iterators from different lists");
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
				if (list != it.list)
					throw std::logic_error("Cannot compare iterators from different lists");
				return node != it.node;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE T &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->data;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->data;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->data;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->data;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator(_first, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator begin_reversed() {
			return Iterator(_last, this, IteratorDirection::Reversed);
		}
		PEFF_FORCEINLINE Iterator end_reversed() {
			return Iterator(nullptr, this, IteratorDirection::Reversed);
		}

		struct ConstIterator {
			const Node *node;
			const List<T> *list;
			IteratorDirection direction;

			PEFF_FORCEINLINE ConstIterator(
				const Node *node,
				const List<T> *list,
				IteratorDirection direction)
				: node(node),
				  list(list),
				  direction(direction) {}

			ConstIterator(const ConstIterator &it) = default;
			PEFF_FORCEINLINE ConstIterator(ConstIterator &&it) {
				node = it.node;
				list = it.list;
				direction = it.direction;

				it.node = nullptr;
				it.list = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PEFF_FORCEINLINE ConstIterator &operator=(const ConstIterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				peff::construct_at<ConstIterator>(this, rhs);
				return *this;
			}
			PEFF_FORCEINLINE ConstIterator &operator=(ConstIterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				peff::construct_at<ConstIterator>(this, std::move(rhs));
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator(const Iterator &it) {
				(*this) = it;
			}
			PEFF_FORCEINLINE ConstIterator(Iterator &&it) {
				(*this) = it;
			}
			PEFF_FORCEINLINE ConstIterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				list = rhs.list;
				return *this;
			}
			PEFF_FORCEINLINE ConstIterator &operator=(Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				node = rhs.node;
				list = rhs.list;
				return *this;
			}

			PEFF_FORCEINLINE bool copy(ConstIterator &dest) noexcept {
				dest = *this;
				return true;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				if (!node)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					node = node->next;
				} else {
					node = node->prev;
				}

				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator++(int) {
				ConstIterator it = *this;
				++(*this);
				return it;
			}

			PEFF_FORCEINLINE ConstIterator &operator--() {
				if (direction == IteratorDirection::Forward) {
					if (!(node = node->prev))
						throw std::logic_error("Dereasing the begin iterator");
				} else {
					if (!(node = node->next))
						throw std::logic_error("Dereasing the begin iterator");
				}

				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator--(int) {
				ConstIterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const Node *node) const noexcept {
				return node == node;
			}

			PEFF_FORCEINLINE bool operator==(const ConstIterator &it) const {
				if (list != it.list)
					throw std::logic_error("Cannot compare iterators from different lists");
				return node == it.node;
			}

			PEFF_FORCEINLINE bool operator==(ConstIterator &&rhs) const {
				const ConstIterator it = rhs;
				return *this == it;
			}

			PEFF_FORCEINLINE bool operator!=(const Node *node) const noexcept {
				return node != node;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &it) const {
				if (list != it.list)
					throw std::logic_error("Cannot compare iterators from different lists");
				return node != it.node;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				ConstIterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE const T &operator*() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->data;
			}

			PEFF_FORCEINLINE const T &operator*() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return node->data;
			}

			PEFF_FORCEINLINE const T *operator->() {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->data;
			}

			PEFF_FORCEINLINE const T *operator->() const {
				if (!node)
					throw std::logic_error("Deferencing the end iterator");
				return &node->data;
			}
		};

		PEFF_FORCEINLINE ConstIterator begin_const() const noexcept {
			return ConstIterator((Node *)_first, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE ConstIterator end_const() const noexcept {
			return ConstIterator(nullptr, this, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE ConstIterator begin_const_reversed() const noexcept {
			return ConstIterator((Node *)_last, this, IteratorDirection::Reversed);
		}
		PEFF_FORCEINLINE ConstIterator end_const_reversed() const noexcept {
			return ConstIterator(nullptr, this, IteratorDirection::Reversed);
		}

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			return begin_const();
		}
		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return end_const();
		}

		PEFF_FORCEINLINE Alloc *allocator() const {
			return _allocator.get();
		}

		PEFF_FORCEINLINE void replace_allocator(Alloc *rhs) noexcept {
			verify_replaceable(_allocator.get(), rhs);

			_allocator = rhs;
		}
	};
}

#endif
