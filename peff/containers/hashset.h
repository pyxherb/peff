#ifndef _PEFF_CONTAINERS_HASHSET_H_
#define _PEFF_CONTAINERS_HASHSET_H_

#include "list.h"
#include "dynarray.h"
#include <peff/utils/option.h>
#include <peff/utils/fallible_cmp.h>
#include <peff/utils/fallible_hash.h>
#include "misc.h"
#include <peff/utils/hash.h>
#include <peff/base/scope_guard.h>
#include <stdexcept>

#if PEFF_CPLUSPLUS >= 202002L
	#include <concepts>
#endif

namespace peff {
	namespace details {
		template <typename T, typename V = void>
		struct HashCodeResultTypeExtractor {
			using type = T;
		};

		template <typename T>
		struct HashCodeResultTypeExtractor<T, std::void_t<decltype(std::declval<T>().value())>> {
			using type = typename T::value_type;
		};
	}
	template <
		typename T,
		typename EqCmp = std::equal_to<T>,
		typename Hasher = peff::Hasher<T>>
	PEFF_REQUIRES_CONCEPT(std::invocable<EqCmp, const T &, const T &>)
	class HashSet {
	public:
		static_assert(std::is_move_constructible_v<T>, "The element must be move-constructible");
		using HasherResult = decltype(std::declval<Hasher>()(std::declval<T>()));
		using HashCode = typename details::HashCodeResultTypeExtractor<HasherResult>::type;

		struct Element {
			T data;
			HashCode hash_code;

			PEFF_FORCEINLINE Element(T &&data, HashCode hash_code) : data(std::move(data)), hash_code(hash_code) {}

			Element(Element &&rhs) = default;
			Element &operator=(Element &&rhs) = default;
		};
		using Bucket = List<Element>;

	private:
		using ThisType = HashSet<T, EqCmp, Hasher>;

		using BucketsType = DynArray<Bucket>;
		BucketsType _buckets;

		size_t _size = 0;
		EqCmp _eq_cmp;
		Hasher _hasher;

		PEFF_FORCEINLINE int _check_capacity() {
			size_t capacity = _buckets.size() << 1;

			if (capacity < _size)
				return 1;
			if (auto size = (capacity >> 1); size > _size) {
				if (size <= 1)
					return 0;
				return -1;
			}
			return 0;
		}

		[[nodiscard]] PEFF_FORCEINLINE static bool _resize_buckets(size_t new_size, BucketsType &old_buckets, BucketsType &new_buckets) {
			{
				if (!new_buckets.resize_uninit(new_size)) {
					return false;
				}
				for (size_t i = 0; i < new_buckets.size(); ++i) {
					peff::construct_at<Bucket>(&new_buckets.at(i), new_buckets.allocator());
				}
			}

			const size_t num_old_buckets = old_buckets.size();
			ScopeGuard restore_guard([new_size, &old_buckets, &new_buckets, num_old_buckets]() noexcept {
				for (size_t i = 0; i < new_size; ++i) {
					Bucket &bucket = new_buckets.at(i);

					for (typename Bucket::NodeHandle j = bucket.first_node(); j; j = j->next) {
						size_t index = ((size_t)j->data.hash_code) % num_old_buckets;

						bucket.detach(j);
						old_buckets.at(index).push_front(j);
					}
				}
				new_buckets.clear_and_shrink();
			});

			for (size_t i = 0; i < num_old_buckets; ++i) {
				Bucket &bucket = old_buckets.at(i);

				for (typename Bucket::NodeHandle j = bucket.first_node(); j;) {
					typename Bucket::NodeHandle next = j->next;
					size_t index = ((size_t)j->data.hash_code) % new_size;

					bucket.detach(j);
					new_buckets.at(index).push_front(j);
					j = next;
				}
			}

			restore_guard.release();

			return true;
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle _get_bucket_slot_alt(const Bucket &bucket, const U &data) const {
			for (auto i = bucket.first_node(); i; i = i->next) {
				if (_eq_cmp(i->data.data, data)) {
					return i;
				}
			}

			return Bucket::null_node_handle();
		}

		[[nodiscard]] PEFF_FORCEINLINE bool _check_and_resize_buckets() {
			size_t size = _buckets.size();

			switch (_check_capacity()) {
				case 1: {
					BucketsType new_buckets(_buckets.allocator());
					if (!_resize_buckets(size ? size << 1 : 1, _buckets, new_buckets)) {
						return false;
					}
					_buckets = std::move(new_buckets);
					break;
				}
				case 0:
					break;
				case -1: {
					BucketsType new_buckets(_buckets.allocator());
					if (!_resize_buckets(size >> 1, _buckets, new_buckets)) {
						return false;
					}
					_buckets = std::move(new_buckets);
					break;
				}
			}

			return true;
		}

		PEFF_FORCEINLINE bool is_buckets_inited() {
			return _buckets.size();
		}

		/// @brief Insert a new element.
		/// @param buckets Buckets to be operated.
		/// @param data Element to insert.
		/// @return true for succeeded, false if failed.
		[[nodiscard]] PEFF_FORCEINLINE T *_insert(T &&data, bool force_resize_buckets) {
			if (!_buckets.size()) {
				if (!_buckets.push_back(Bucket(_buckets.allocator()))) {
					return nullptr;
				}
			}

			T tmp_data = std::move(data);

			HashCode hash_code = _hasher(tmp_data);
			size_t index = ((size_t)hash_code) % _buckets.size();
			Bucket &bucket = _buckets.at(index);

			T *result = nullptr;
			for (auto &i : bucket) {
				if (_eq_cmp(i.data, tmp_data)) {
					move_assign_or_move_construct<T>(i.data, std::move(tmp_data));
					result = &i.data;
					goto inserted;
				}
			}
			{
				auto h = bucket.push_front(Element(std::move(tmp_data), hash_code));
				if (!h)
					return nullptr;
				result = &h->data.data;
			}

		inserted:
			if (!_check_and_resize_buckets()) {
				if (force_resize_buckets) {
					bucket.pop_front();
					return nullptr;
				}
			}

			++_size;
			return result;
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE peff::Option<T> _remove_alt(const U &data, bool force_resize_buckets) {
			if (!_buckets.size()) {
				return peff::NULLOPT;
			}

			HashCode hash_code = _hasher(data);
			size_t index = ((size_t)hash_code) % _buckets.size();
			Bucket &bucket = _buckets.at(index);
			peff::RcObjectPtr<Alloc> alloc = bucket.allocator();

			typename Bucket::NodeHandle node;

			node = _get_bucket_slot_alt(bucket, data);

			if (node) {
				typename Bucket::NodeHandle next_node = Bucket::next(node, 1);

				bucket.detach(node);

				T data = std::move(node->data.data);
				bucket.delete_node(node);

				--_size;
				return std::move(data);
			}
			return peff::NULLOPT;
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle _get_alt(PEFF_IN_REF const U &data, size_t &index_out) const {
			if (!_buckets.size()) {
				return Bucket::null_node_handle();
			}

			HashCode hash_code = _hasher(data);
			size_t i = ((size_t)hash_code) % _buckets.size();
			const Bucket &bucket = _buckets.at(i);

			index_out = i;

			return _get_bucket_slot_alt<U>(bucket, data);
		}

	public:
		PEFF_FORCEINLINE HashSet(PEFF_NONNULL Alloc *allocator) : _buckets(allocator) {
		}

		PEFF_FORCEINLINE HashSet(ThisType &&other)
			: _buckets(std::move(other._buckets)),
			  _size(other._size),
			  _eq_cmp(std::move(other._eq_cmp)),
			  _hasher(std::move(other._hasher)) {
			other._size = 0;
		}

		PEFF_FORCEINLINE ThisType &operator=(ThisType &&other) noexcept {
			clear_and_shrink();

			_buckets = std::move(other._buckets);
			_size = other._size;
			_eq_cmp = std::move(other._eq_cmp);
			_hasher = std::move(other._hasher);

			other._size = 0;

			return *this;
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert_without_resize_buckets(T &&data) {
			return _insert(std::move(data), false);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool insert(T &&data) {
			return _insert(std::move(data), true);
		}

		PEFF_FORCEINLINE peff::Option<T> remove(const T &data) {
			return _remove_alt<T>(data, false);
		}

		template <typename U>
		PEFF_FORCEINLINE peff::Option<T> remove_alt(const U &data) {
			return _remove_alt<U>(data, false);
		}

		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle get(const T &data) {
			size_t index;
			return _get_alt<T>(data, index);
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE typename Bucket::NodeHandle get_alt(const U &data) {
			size_t index;
			return _get_alt<U>(data, index);
		}

		[[nodiscard]] PEFF_FORCEINLINE bool contains(const T &data) const {
			size_t index;
			return _get_alt<T>(data, index);
		}

		template <typename U>
		[[nodiscard]] PEFF_FORCEINLINE bool contains(const U &data) const {
			size_t index;
			return _get_alt<U>(data, index);
		}

		PEFF_FORCEINLINE void clear() {
			_buckets.clear();
		}

		PEFF_FORCEINLINE void clear_and_shrink() {
			_buckets.clear_and_shrink();
		}

		PEFF_FORCEINLINE PEFF_NONNULL_RET Alloc *allocator() const {
			return _buckets.allocator();
		}

		PEFF_FORCEINLINE void replace_allocator(PEFF_NONNULL Alloc *rhs) noexcept {
			for (auto &i : _buckets) {
				i.replace_allocator(rhs);
			}
			_buckets.replace_allocator(rhs);
		}

		struct Iterator {
			size_t idx_cur_bucket;
			typename Bucket::NodeHandle bucket_node_handle;
			ThisType *hash_set;
			IteratorDirection direction;

			PEFF_FORCEINLINE Iterator(
				ThisType *hash_set,
				size_t idx_cur_bucket,
				typename Bucket::NodeHandle bucket_node_handle,
				IteratorDirection direction)
				: idx_cur_bucket(idx_cur_bucket),
				  bucket_node_handle(bucket_node_handle),
				  hash_set(hash_set),
				  direction(direction) {}

			Iterator(const Iterator &it) = default;
			PEFF_FORCEINLINE Iterator(Iterator &&it) {
				idx_cur_bucket = it.idx_cur_bucket;
				bucket_node_handle = it.bucket_node_handle;
				hash_set = it.hash_set;
				direction = it.direction;

				it.idx_cur_bucket = SIZE_MAX;
				it.bucket_node_handle = Bucket::null_node_handle();
				it.hash_set = nullptr;
				it.direction = IteratorDirection::Invalid;
			}
			PEFF_FORCEINLINE Iterator &operator=(const Iterator &rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				idx_cur_bucket = rhs.idx_cur_bucket;
				bucket_node_handle = rhs.bucket_node_handle;
				hash_set = rhs.hash_set;
				return *this;
			}
			PEFF_FORCEINLINE Iterator &operator=(Iterator &&rhs) noexcept {
				if (direction != rhs.direction)
					throw std::logic_error("Incompatible iterator direction");
				peff::construct_at<Iterator>(this, std::move(rhs));
				return *this;
			}

			PEFF_FORCEINLINE Iterator &operator++() {
				if (idx_cur_bucket == SIZE_MAX)
					throw std::logic_error("Increasing the end iterator");

				if (direction == IteratorDirection::Forward) {
					typename Bucket::NodeHandle next_node = Bucket::next(bucket_node_handle, 1);
					if (!next_node) {
						while ((!next_node) && (idx_cur_bucket != SIZE_MAX)) {
							if (++idx_cur_bucket >= hash_set->_buckets.size()) {
								idx_cur_bucket = SIZE_MAX;
								next_node = nullptr;
							} else {
								next_node = hash_set->_buckets.at(idx_cur_bucket).first_node();
							}
						}
					}
					bucket_node_handle = next_node;
				} else {
					typename Bucket::NodeHandle next_node = Bucket::prev(bucket_node_handle, 1);
					if (!next_node) {
						while ((!next_node) && (idx_cur_bucket != SIZE_MAX)) {
							if (!idx_cur_bucket) {
								idx_cur_bucket = SIZE_MAX;
								next_node = nullptr;
							} else {
								--idx_cur_bucket;
								next_node = hash_set->_buckets.at(idx_cur_bucket).last_node();
							}
						}
					}
					bucket_node_handle = next_node;
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
					if (idx_cur_bucket == SIZE_MAX) {
						idx_cur_bucket = hash_set->_buckets.size();
						bucket_node_handle = hash_set->_buckets.at(idx_cur_bucket).last_node();
					} else {
						typename Bucket::NodeHandle next_node = Bucket::prev(bucket_node_handle, 1);
						if (!next_node) {
							while (!next_node) {
								if (!idx_cur_bucket) {
									throw std::logic_error("Decreasing the beginning iterator");
								} else {
									--idx_cur_bucket;
									next_node = hash_set->_buckets.at(idx_cur_bucket).last_node();
								}
							}
						}
						bucket_node_handle = next_node;
					}
				} else {
					if (idx_cur_bucket == SIZE_MAX) {
						idx_cur_bucket = 0;
						bucket_node_handle = hash_set->_buckets.at(0).first_node();
					} else {
						typename Bucket::NodeHandle next_node = Bucket::next(bucket_node_handle, 1);
						if (!next_node) {
							while (!next_node) {
								if (++idx_cur_bucket >= hash_set->_buckets.size()) {
									throw std::logic_error("Decreasing the beginning iterator");
								} else {
									next_node = hash_set->_buckets.at(idx_cur_bucket).first_node();
								}
							}
						}
						bucket_node_handle = next_node;
					}
				}

				return *this;
			}

			PEFF_FORCEINLINE Iterator operator--(int) {
				Iterator it = *this;
				--(*this);
				return it;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &it) const {
				if (hash_set != it.hash_set)
					throw std::logic_error("Cannot compare iterators from different containers");
				return bucket_node_handle == it.bucket_node_handle;
			}

			PEFF_FORCEINLINE bool operator==(const Iterator &&rhs) const {
				const Iterator it = rhs;
				return *this == it;
			}

			PEFF_FORCEINLINE bool operator!=(const Iterator &it) const {
				if (hash_set != it.hash_set)
					throw std::logic_error("Cannot compare iterators from different containers");
				return bucket_node_handle != it.bucket_node_handle;
			}

			PEFF_FORCEINLINE bool operator!=(Iterator &&rhs) const {
				Iterator it = rhs;
				return *this != it;
			}

			PEFF_FORCEINLINE T &operator*() {
				if (!bucket_node_handle)
					throw std::logic_error("Deferencing the end iterator");
				return bucket_node_handle->data.data;
			}

			PEFF_FORCEINLINE T &operator*() const {
				if (!bucket_node_handle)
					throw std::logic_error("Deferencing the end iterator");
				return bucket_node_handle->data.data;
			}

			PEFF_FORCEINLINE T *operator->() {
				if (!bucket_node_handle)
					throw std::logic_error("Deferencing the end iterator");
				return &bucket_node_handle->data.data;
			}

			PEFF_FORCEINLINE T *operator->() const {
				if (!bucket_node_handle)
					throw std::logic_error("Deferencing the end iterator");
				return &bucket_node_handle->data.data;
			}
		};

		PEFF_FORCEINLINE Iterator begin() {
			for (size_t i = 0; i < _buckets.size(); ++i) {
				auto &cur_bucket = _buckets.at(i);
				typename Bucket::NodeHandle node = cur_bucket.first_node();
				if (node) {
					return Iterator(this, i, node, IteratorDirection::Forward);
				}
			}
			return end();
		}
		PEFF_FORCEINLINE Iterator end() {
			return Iterator(this, SIZE_MAX, nullptr, IteratorDirection::Forward);
		}
		PEFF_FORCEINLINE Iterator rbegin() {
			for (size_t i = _buckets.size(); i; --i) {
				auto &cur_bucket = _buckets.at(i);
				typename Bucket::NodeHandle node = cur_bucket.last_node();
				if (node) {
					return Iterator(this, i, node, IteratorDirection::Reversed);
				}
			}

			auto &cur_bucket = _buckets.at(0);
			typename Bucket::NodeHandle node = cur_bucket.last_node();
			if (node) {
				return Iterator(this, 0, node, IteratorDirection::Reversed);
			}
			return rend();
		}

		PEFF_FORCEINLINE Iterator rend() {
			return Iterator(this, SIZE_MAX, nullptr, IteratorDirection::Reversed);
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
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE bool operator!=(ConstIterator &&rhs) const {
				return _iterator == rhs._iterator;
			}

			PEFF_FORCEINLINE T &operator*() {
				return *_iterator;
			}

			PEFF_FORCEINLINE const T &operator*() const {
				return *_iterator;
			}

			PEFF_FORCEINLINE T *operator->() {
				return &*_iterator;
			}

			PEFF_FORCEINLINE const T *operator->() const {
				return &*_iterator;
			}

			PEFF_FORCEINLINE ConstIterator &operator++() {
				++_iterator;
				return *this;
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

		PEFF_FORCEINLINE Iterator find(const T &value) {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<T>(value, index);
			if (!node)
				return end();
			return Iterator(this, index, node, IteratorDirection::Forward);
		}

		template <typename U>
		PEFF_FORCEINLINE Iterator find_alt(const U &value) {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<U>(value, index);
			if (!node)
				return end();
			return Iterator(this, index, node, IteratorDirection::Forward);
		}

		PEFF_FORCEINLINE ConstIterator find(const T &value) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(value));
		}

		template <typename U>
		PEFF_FORCEINLINE ConstIterator find_alt(const U &value) const {
			return ConstIterator(const_cast<ThisType *>(this)->find(value));
		}

		PEFF_FORCEINLINE T &at(const T &value) {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<T>(value, index);
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		template <typename U>
		PEFF_FORCEINLINE T &at_alt(const U &value) {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<U>(value, index);
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		PEFF_FORCEINLINE const T &at(const T &value) const noexcept {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<T>(value, index);
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		template <typename U>
		PEFF_FORCEINLINE const T &at_alt(const U &value) const noexcept {
			size_t index;
			typename Bucket::NodeHandle node = _get_alt<U>(value, index);
			if (!node)
				throw std::out_of_range("No such element");
			return node->data.data;
		}

		PEFF_FORCEINLINE size_t size() const noexcept {
			return _size;
		}

		PEFF_FORCEINLINE bool empty() const noexcept {
			return !_size;
		}

		PEFF_FORCEINLINE bool shrink_buckets() {
			return _buckets.shrink_to_fit();
		}
	};
}

#endif
