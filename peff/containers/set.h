#ifndef _PEFF_CONTAINERS_SET_H_
#define _PEFF_CONTAINERS_SET_H_

#include "rbtree.h"

namespace peff {
	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> || std::invocable<Comparator, T &&, T &&>)
	class Set final {
	private:
		static_assert(std::is_move_constructible_v<T>, "The element must be move-constructible");
		using Tree = RBTree<T, Comparator, IsThreeway>;
		Tree _tree;
		using ThisType = Set<T, Comparator, IsThreeway>;

	public:
		using NodeType = typename Tree::NodeType;

		PEFF_FORCEINLINE Set(PEFF_NONNULL Alloc *allocator, Comparator &&comparator = {}) : _tree(allocator, std::move(comparator)) {
		}
		PEFF_FORCEINLINE Set(ThisType &&rhs) : _tree(std::move(rhs._tree)) {
		}
		PEFF_FORCEINLINE ~Set() {
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&dest) noexcept {
			_tree = std::move(dest._tree);
			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE T *insert(T &&value) {
			typename Tree::Node *node = _tree.insert(std::move(value));

			if (!node)
				return nullptr;

			return &node->rb_value;
		}

		PEFF_FORCEINLINE peff::Option<T> remove(const T &key) {
			return remove_alt<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE peff::Option<T> remove_alt(const U &key) {
			auto node = _tree.template get_alt<U>(key);

			if (node)
				return _tree.remove(node);

			return peff::NULLOPT;
		}

		PEFF_FORCEINLINE void verify() {
			_tree.verify();
		}

		PEFF_FORCEINLINE size_t size() const {
			return _tree.size();
		}

		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return _tree.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
			_tree.replace_allocator(rhs);
		}

		PEFF_FORCEINLINE Comparator &comparator() {
			return _tree.comparator();
		}

		PEFF_FORCEINLINE const Comparator &comparator() const {
			return _tree.comparator();
		}

		PEFF_FORCEINLINE void clear() {
			_tree.clear();
		}

		PEFF_FORCEINLINE T &at(const T &key) {
			return at_alt<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE T &at_alt(const U &key) {
			auto node = _tree.template get_alt<U>(key);

			PEFF_ASSERT(node, "Corresponding element not found!");

			return node->rb_value;
		}

		PEFF_FORCEINLINE const T &at(const T &key) const {
			return at_alt<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE const T &at_alt(const U &key) const {
			auto node = _tree.template get_alt<U>(key);

			assert(node);

			return node->rb_value;
		}

		struct Iterator {
			typename Tree::Iterator _iterator;
			PEFF_FORCEINLINE Iterator(typename Tree::Iterator &&iterator_in) : _iterator(iterator_in) {
			}
			Iterator(const Iterator &rhs) = default;
			Iterator(Iterator &&rhs) = default;
			Iterator &operator=(const Iterator &rhs) = default;
			Iterator &operator=(Iterator &&rhs) = default;

			PEFF_FORCEINLINE bool operator==(const Iterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(Iterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator++(int) {
				Iterator it = *this;
				++*this;
				return it;
			}

			PEFF_FORCEINLINE Iterator next() {
				Iterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE Iterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--*this;
				return it;
			}

			PEFF_FORCEINLINE Iterator prev() {
				Iterator iterator = *this;

				return --iterator;
			}

			PEFF_FORCEINLINE T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE T *operator->() const {
				return &*_iterator;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			return Iterator(_tree.begin());
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(_tree.end());
		}
		PEFF_FORCEINLINE Iterator rbegin() {
			return Iterator(_tree.rbegin());
		}
		PEFF_FORCEINLINE Iterator rend() {
			return Iterator(_tree.rend());
		}

		struct ConstIterator {
			Iterator _iterator;
			PEFF_FORCEINLINE ConstIterator(Iterator &&iterator_in) : _iterator(iterator_in) {
			}
			ConstIterator(const ConstIterator &rhs) = default;
			ConstIterator(ConstIterator &&rhs) = default;
			ConstIterator &operator=(const ConstIterator &rhs) = default;
			ConstIterator &operator=(ConstIterator &&rhs) = default;

			PEFF_FORCEINLINE bool operator==(const ConstIterator &rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator==(ConstIterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(const ConstIterator &rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				return _iterator != rhs._iterator;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator++(int) {
				return _iterator++;
			}

			PEFF_FORCEINLINE ConstIterator &operator--() {
				--_iterator;
				return *this;
			}

			PEFF_FORCEINLINE ConstIterator operator--(int) {
				return _iterator--;
			}

			PEFF_FORCEINLINE ConstIterator next() {
				ConstIterator iterator = *this;

				return ++iterator;
			}

			PEFF_FORCEINLINE ConstIterator prev() {
				ConstIterator iterator = *this;

				return --iterator;
			}

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator->() const {
				return *_iterator;
			}
		};

		PEFF_FORCEINLINE ConstIterator begin() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->begin());
		}
		PEFF_FORCEINLINE ConstIterator end() const noexcept {
			return ConstIterator(const_cast<ThisType *>(this)->end());
		}
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

		PEFF_FORCEINLINE bool contains(const T &key) const {
			return contains_alt<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE bool contains_alt(const U &key) const {
			return _tree.template get_alt<U>(key);
		}
		PEFF_FORCEINLINE ConstIterator find(const T &key) const {
			return const_cast<ThisType *>(this)->find(key);
		}
		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_alt(const U &key) const {
			return const_cast<ThisType *>(this)->find_alt<U>(key);
		}

		PEFF_FORCEINLINE Iterator find(const T &key) {
			return find_alt<T>(key);
		}
		template <typename U>
		PEFF_FORCEINLINE Iterator find_alt(const U &key) {
			if (auto node = _tree.template get_alt<U>(key); node) {
				return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE Iterator find_max_lteq(const T &key) {
			return find_max_lteq_alt<T>(key);
		}

		template <typename U>
		PEFF_FORCEINLINE Iterator find_max_lteq_alt(const U &key) {
			if (auto node = _tree.template get_max_lteq_alt<U>(key); node) {
				return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
			}
			return _tree.end();
		}

		PEFF_FORCEINLINE ConstIterator find_max_lteq(const T &key) const {
			return const_cast<ThisType *>(this)->find_max_lteq(key);
		}

		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_max_lteq_alt(const U &key) const {
			return const_cast<ThisType *>(this)->find_max_lteq_alt(key);
		}

		PEFF_FORCEINLINE peff::Option<T> remove(const Iterator &iterator) {
			return _tree.remove(iterator._iterator);
		}
	};
}

#endif
