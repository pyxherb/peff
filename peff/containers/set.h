#ifndef _PEFF_CONTAINERS_SET_H_
#define _PEFF_CONTAINERS_SET_H_

#include "rbtree.h"
#include <peff/utils/result.h>

namespace peff {
	/// @brief An ordered set backed by a red-black tree.
	/// @tparam T The type of the stored elements.
	/// @tparam Comparator The type of the comparator used to order the elements.
	/// @tparam IsThreeway Whether the comparator yields a three-way ordering.
	/// @tparam Fallible Whether the comparator is allowed to fail. When enabled together with
	///         IsThreeway, the comparator must return peff::PartialOrd, and every operation that
	///         compares keys may fail with peff::Unordered, which is reported through the returned
	///         peff::Result.
	template <typename T, typename Comparator = std::less<T>, bool IsThreeway = false, bool Fallible = false>
	PEFF_REQUIRES_CONCEPT(std::invocable<Comparator, const T &, const T &> || std::invocable<Comparator, T &&, T &&>)
	class Set final {
	private:
		static_assert(std::is_move_constructible_v<T>, "The element must be move-constructible");
		using Tree = RBTreeImpl<T, Comparator, Fallible, IsThreeway>;
		Tree _tree;
		using ThisType = Set<T, Comparator, IsThreeway, Fallible>;

		/// @brief Extract the raw node pointer from an optional node.
		/// @param opt The optional node produced by a query.
		/// @return The node pointer, or null when there is none.
		[[nodiscard]] PEFF_FORCEINLINE static PEFF_NULLABLE_RET typename Tree::Node *_unwrap(PEFF_IN const Option<typename Tree::Node *> &opt) noexcept {
			return opt.has_value() ? opt.value() : nullptr;
		}

	public:
		using NodeType = typename Tree::NodeType;

		/// @brief The result of an operation that may fail to compare two keys. In the fallible mode
		///        it is a peff::Result whose error type is peff::Unordered, otherwise it is the plain
		///        value type of the operation.
		template <typename X>
		using Outcome = std::conditional_t<Fallible, Result<X, Unordered>, X>;

		/// @brief The result of a lookup that yields a reference to the stored element.
		using AtResult = std::conditional_t<Fallible, Result<T *, Unordered>, T &>;

		/// @brief The result of a lookup that yields an immutable reference to the stored element.
		using ConstAtResult = std::conditional_t<Fallible, Result<const T *, Unordered>, const T &>;

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

		/// @brief Insert an element into the set.
		/// @param value Element to be inserted, must be an rvalue.
		/// @return Pointer to the stored element on success. On failure it is either a null pointer
		///         (out of memory) or, in the fallible mode, an Unordered error.
		[[nodiscard]] PEFF_FORCEINLINE Outcome<T *> insert(T &&value) {
			if constexpr (Fallible) {
				auto result = _tree.insert(std::move(value));

				if (result.is_error())
					return Unordered();

				typename Tree::Node *node = result.value();

				return node ? &node->rb_value : nullptr;
			} else {
				typename Tree::Node *node = _tree.insert(std::move(value));

				return node ? &node->rb_value : nullptr;
			}
		}

		/// @brief Remove an element from the set.
		/// @param key Key of the element to be removed.
		/// @return Removed element, an empty option when there is no such element, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Option<T>> remove(const T &key) {
			return remove_alt<T>(key);
		}

		/// @brief Remove an element from the set, using an alternative-typed key.
		/// @param key Alternative-typed key of the element to be removed.
		/// @return Removed element, an empty option when there is no such element, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Option<T>> remove_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _tree.template get_alt<U>(key);

				if (found.is_error())
					return Unordered();

				typename Tree::Node *node = _unwrap(found.value());

				if (node)
					return _tree.remove(node);
				return Option<T>(NULLOPT);
			} else {
				auto node = _tree.template get_alt<U>(key);

				if (node)
					return _tree.remove(node);

				return peff::NULLOPT;
			}
		}

		PEFF_FORCEINLINE void verify() {
			_tree.verify();
		}

		PEFF_FORCEINLINE size_t size() const {
			return _tree.size();
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
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

		/// @brief Get reference to the element that is associated to a key.
		/// @param key Key to the element.
		/// @return Reference to the stored element, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		PEFF_FORCEINLINE AtResult at(const T &key) {
			return at_alt<T>(key);
		}

		/// @brief Get reference to the element that is associated to a key.
		/// @param key Key to the element.
		/// @return Reference to the stored element, or, in the fallible mode, an Unordered error
		///         when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE AtResult at_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _tree.template get_alt<U>(key);

				if (found.is_error())
					return Unordered();

				typename Tree::Node *node = _unwrap(found.value());

				PEFF_ASSERT(node, "Corresponding element not found!");

				return node ? &node->rb_value : nullptr;
			} else {
				typename Tree::Node *node = _tree.template get_alt<U>(key);

				PEFF_ASSERT(node, "Corresponding element not found!");

				return node->rb_value;
			}
		}

		/// @brief Get immutable reference to the element that is associated to a key.
		/// @param key Key to the element.
		/// @return Immutable reference to the stored element, or, in the fallible mode, an Unordered
		///         error when the comparison failed.
		PEFF_FORCEINLINE ConstAtResult at(const T &key) const {
			return at_alt<T>(key);
		}

		/// @brief Get immutable reference to the element that is associated to a key, using an
		///        alternative-typed key.
		/// @param key Alternative-typed key to the element.
		/// @return Immutable reference to the stored element, or, in the fallible mode, an Unordered
		///         error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE ConstAtResult at_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = _tree.template get_alt<U>(key);

				if (found.is_error())
					return Unordered();

				typename Tree::Node *node = _unwrap(found.value());

				PEFF_ASSERT(node, "Corresponding element not found!");

				return node ? &node->rb_value : nullptr;
			} else {
				typename Tree::Node *node = _tree.template get_alt<U>(key);

				PEFF_ASSERT(node, "Corresponding element not found!");

				return node->rb_value;
			}
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

		/// @brief Check if an element is already contained in the set.
		/// @param key Key of the element to be checked.
		/// @return true when the element is contained, false otherwise, or, in the fallible mode,
		///         an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<bool> contains(const T &key) const {
			return contains_alt<T>(key);
		}

		/// @brief Check if an element is already contained in the set, using an alternative-typed key.
		/// @param key Alternative-typed key of the element to be checked.
		/// @return true when the element is contained, false otherwise, or, in the fallible mode,
		///         an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<bool> contains_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = _tree.template get_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return _unwrap(found.value()) != nullptr;
			} else {
				return _tree.template get_alt<U>(key);
			}
		}

		/// @brief Find an element in the set.
		/// @param key Key of the element to be found.
		/// @return Iterator to the element, an end iterator when it is not present, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Iterator> find(const T &key) {
			return find_alt<T>(key);
		}

		/// @brief Find an element in the set, using an alternative-typed key.
		/// @param key Alternative-typed key of the element to be found.
		/// @return Iterator to the element, an end iterator when it is not present, or, in the
		///         fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _tree.template get_alt<U>(key);

				if (found.is_error())
					return Unordered();

				typename Tree::Node *node = _unwrap(found.value());

				if (node)
					return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
				return Iterator(_tree.end());
			} else {
				if (auto node = _tree.template get_alt<U>(key); node)
					return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
				return Iterator(_tree.end());
			}
		}

		/// @brief Find an element in the set.
		/// @param key Key of the element to be found.
		/// @return Immutable iterator to the element, an end iterator when it is not present, or,
		///         in the fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<ConstIterator> find(const T &key) const {
			return find_alt<T>(key);
		}

		/// @brief Find an element in the set, using an alternative-typed key.
		/// @param key Alternative-typed key of the element to be found.
		/// @return Immutable iterator to the element, an end iterator when it is not present, or,
		///         in the fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->template find_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->template find_alt<U>(key));
			}
		}

		/// @brief Find the element with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Iterator to the element, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq(const T &key) {
			return find_max_lteq_alt<T>(key);
		}

		/// @brief Find the element with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @param key Alternative-typed key for query.
		/// @return Iterator to the element, an end iterator when there is none, or, in the fallible
		///         mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<Iterator> find_max_lteq_alt(const U &key) {
			if constexpr (Fallible) {
				auto found = _tree.template get_max_lteq_alt<U>(key);

				if (found.is_error())
					return Unordered();

				typename Tree::Node *node = _unwrap(found.value());

				if (node)
					return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
				return Iterator(_tree.end());
			} else {
				if (auto node = _tree.template get_max_lteq_alt<U>(key); node)
					return Iterator(typename Tree::Iterator(node, &_tree, IteratorDirection::Forward));
				return Iterator(_tree.end());
			}
		}

		/// @brief Find the element with the greatest key that is not greater than the given key.
		/// @param key Key for query.
		/// @return Immutable iterator to the element, an end iterator when there is none, or, in
		///         the fallible mode, an Unordered error when the comparison failed.
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq(const T &key) const {
			return find_max_lteq_alt<T>(key);
		}

		/// @brief Find the element with the greatest key that is not greater than the given
		///        alternative-typed key.
		/// @param key Alternative-typed key for query.
		/// @return Immutable iterator to the element, an end iterator when there is none, or, in
		///         the fallible mode, an Unordered error when the comparison failed.
		template <typename U>
		PEFF_FORCEINLINE Outcome<ConstIterator> find_max_lteq_alt(const U &key) const {
			if constexpr (Fallible) {
				auto found = const_cast<ThisType *>(this)->template find_max_lteq_alt<U>(key);

				if (found.is_error())
					return Unordered();

				return ConstIterator(std::move(found.value()));
			} else {
				return ConstIterator(const_cast<ThisType *>(this)->template find_max_lteq_alt<U>(key));
			}
		}

		/// @brief Remove the element pointed to by an iterator from the set.
		/// @param iterator Iterator to the element to be removed.
		/// @return Removed element.
		PEFF_FORCEINLINE peff::Option<T> remove(const Iterator &iterator) {
			return _tree.remove(iterator._iterator);
		}
	};
}

#endif
