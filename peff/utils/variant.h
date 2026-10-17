#ifndef _PEFF_UTILS_VARIANT_H_
#define _PEFF_UTILS_VARIANT_H_

#include "misc.h"
#include <peff/base/alloc.h>
#include <type_traits>
#include <array>

namespace peff {
	template <std::size_t Index, typename T, typename... Ts>
	struct _VariantTagOf;

	template <std::size_t Index, typename T, typename U>
	struct _VariantTagOf<Index, T, U> {
		constexpr static std::size_t value =
			std::is_same_v<T, U> ? Index : Index + 1;
	};

	template <std::size_t Index, typename T, typename U, typename... Ts>
	struct _VariantTagOf<Index, T, U, Ts...> {
		constexpr static std::size_t value =
			std::is_same_v<T, U>
				? Index
				: _VariantTagOf<Index + 1, T, Ts...>::value;
	};

	template <typename... Ts>
	using _IsAllTriviallyDestructible =
		std::conjunction<std::is_trivially_destructible<Ts>...>;

	template <typename... Ts>
	using _IsAllTriviallyCopyable =
		std::conjunction<std::is_trivially_copyable<Ts>...>;

	template <typename... Ts>
	using _IsAllNoexceptMoveable =
		std::conjunction<std::is_nothrow_move_constructible<Ts>...>;

	using VariantIndex = size_t;
	constexpr VariantIndex INVALID_VARIANT_INDEX = SIZE_MAX;

	template <bool TriviallyDestructible, bool TriviallyCopyable, typename... Ts>
	struct _VariantStorage {};

	template <uint8_t Times, typename T, typename... Ts>
	struct _OccurrenceTimes;

	template <uint8_t Times, typename T, typename U>
	struct _OccurrenceTimes<Times, T, U> {
		constexpr static std::size_t value =
			std::is_same_v<T, U> ? Times + 1: Times;
	};

	template <uint8_t Times, typename T, typename U, typename... Ts>
	struct _OccurrenceTimes<Times, T, U, Ts...> {
		constexpr static std::size_t value =
			std::is_same_v<T, U>
				? _OccurrenceTimes<Times + 1, T, Ts...>::value
				: _OccurrenceTimes<Times, T, Ts...>::value;
	};

	template <class Tuple, std::size_t... Indices>
	PEFF_FORCEINLINE bool _move_variant_impl(
		std::size_t index,
		void *buffer,
		void *src,
		std::index_sequence<Indices...>) {
		return (... || (index == Indices &&
						   (peff::construct_at(
								reinterpret_cast<
									std::tuple_element_t<Indices, Tuple> *>(buffer),
								std::move(
									*std::launder(
										reinterpret_cast<
											std::tuple_element_t<Indices, Tuple> *>(src)))),
							   // Destroy the old value.
							   peff::destroy_at(
								   std::launder(
									   reinterpret_cast<
										   std::tuple_element_t<Indices, Tuple> *>(src))),
							   true)));
	}

	template <class... Ts>
	PEFF_FORCEINLINE bool _move_variant(
		std::size_t index,
		void *buffer,
		void *src) {
		return peff::_move_variant_impl<std::tuple<Ts...>>(
			index,
			buffer,
			src,
			std::index_sequence_for<Ts...>{});
	}

	template <class Tuple, std::size_t... Indices>
	PEFF_FORCEINLINE bool _copy_variant_impl(
		std::size_t index,
		void *buffer,
		const void *src,
		std::index_sequence<Indices...>) {
		return (... || (index == Indices &&
						   (peff::construct_at(
								reinterpret_cast<
									std::tuple_element_t<Indices, Tuple> *>(buffer),
								*std::launder(
									reinterpret_cast<
										const std::tuple_element_t<Indices, Tuple> *>(src))),
							   true)));
	}

	template <class... Ts>
	PEFF_FORCEINLINE bool _copy_variant(
		std::size_t index,
		void *buffer,
		const void *src) {
		return peff::_copy_variant_impl<std::tuple<Ts...>>(
			index,
			buffer,
			src,
			std::index_sequence_for<Ts...>{});
	}

	template <class Tuple, std::size_t... Indices>
	void _destroy_variant_impl(
		std::size_t index,
		void *buffer,
		std::index_sequence<Indices...>) {
		(void)(... || (index == Indices &&
						  (std::destroy_at(std::launder(
							   reinterpret_cast<
								   std::tuple_element_t<Indices, Tuple> *>(buffer))),
							  true)));
	}

	template <class... Ts>
	void _destroy_variant(std::size_t index, void *buffer) {
		peff::_destroy_variant_impl<std::tuple<Ts...>>(
			index,
			buffer,
			std::index_sequence_for<Ts...>{});
	}

	template <typename... Ts>
	struct _VariantStorageBase {
	public:
		using TagType = typename AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

		template <typename T>
		constexpr static TagType _TAG_OF = _VariantTagOf<0, T, Ts...>::value;

		alignas(MaxAlignOf<Ts...>::value) char _storage[MaxSizeOf<Ts...>::value];
		TagType _tag;

		PEFF_FORCEINLINE _VariantStorageBase() : _tag(_INVALID_TAG_VALUE) {
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorageBase(const T &data) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(_storage), "Self-assignment is not allowed!");
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<std::decay_t<T> *>(_storage), data);
			_tag = _TAG_OF<T>;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorageBase(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(_storage), "Self-assignment is not allowed!");
			static_assert(_TAG_OF<std::decay_t<T>> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<std::decay_t<T>>, "The type must be move-constructible");
			peff::construct_at<std::decay_t<T>>(reinterpret_cast<std::decay_t<T> *>(_storage), std::forward<T>(data));
			_tag = _TAG_OF<std::decay_t<T>>;
		}
		~_VariantStorageBase() = default;

		template <typename T>
		PEFF_FORCEINLINE T &get() & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *std::launder(reinterpret_cast<std::decay_t<T> *>(_storage));
		}

		template <typename T>
		PEFF_FORCEINLINE const T &get() const & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *std::launder(reinterpret_cast<const T *>(_storage));
		}

		template <typename T>
		PEFF_FORCEINLINE T get() && noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			T v = std::move(*std::launder(reinterpret_cast<std::decay_t<T> *>(_storage)));
			_destroy_variant<Ts...>(_tag, _storage);
			_tag = _INVALID_TAG_VALUE;
			return std::move(v);
		}

		PEFF_FORCEINLINE TagType get_tag() const noexcept {
			return _tag;
		}
	};

	template <typename... Ts>
	struct _VariantStorage<true, false, Ts...> : public _VariantStorageBase<Ts...> {
	private:
		using BaseType = _VariantStorageBase<Ts...>;
		using ThisType = _VariantStorage<true, false, Ts...>;

	public:
		PEFF_FORCEINLINE _VariantStorage(const ThisType &storage) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_copy_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
		}
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) : BaseType(data) {
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) : BaseType(std::forward<T>(data)) {
		}
		~_VariantStorage() = default;

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<T> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			peff::construct_at<T>(reinterpret_cast<std::decay_t<T> *>(this->_storage), data);
			this->_tag = BaseType::template _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<std::decay_t<T>> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<std::decay_t<T>>, "The type must be move-constructible");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			peff::construct_at<std::decay_t<T>>(reinterpret_cast<std::decay_t<T> *>(this->_storage), std::forward<T>(data));
			this->_tag = BaseType::template _TAG_OF<std::decay_t<T>>;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(const ThisType &storage) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			_copy_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			return *this;
		}
	};

	template <typename... Ts>
	struct _VariantStorage<true, true, Ts...> : public _VariantStorageBase<Ts...> {
	private:
		using BaseType = _VariantStorageBase<Ts...>;
		using ThisType = _VariantStorage<true, true, Ts...>;

	public:
		/// @note Every case is trivially copyable, so the byte-wise copy of the whole storage performed by
		/// the defaulted operations is exactly the required behavior, and the storage itself stays trivially
		/// copyable (which the @c true @c true specialization exists for).
		_VariantStorage(const ThisType &storage) = default;
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) : BaseType(data) {
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) : BaseType(std::forward<T>(data)) {
		}
		~_VariantStorage() = default;

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<T> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			peff::construct_at<T>(reinterpret_cast<std::decay_t<T> *>(this->_storage), data);
			this->_tag = BaseType::template _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<std::decay_t<T>> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<std::decay_t<T>>, "The type must be move-constructible");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			peff::construct_at<std::decay_t<T>>(reinterpret_cast<std::decay_t<T> *>(this->_storage), std::forward<T>(data));
			this->_tag = BaseType::template _TAG_OF<std::decay_t<T>>;
			return *this;
		}
		ThisType &operator=(const ThisType &) = default;
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
			return *this;
		}
	};

	template <typename... Ts>
	struct _VariantStorage<false, false, Ts...> : public _VariantStorageBase<Ts...> {
	private:
		using BaseType = _VariantStorageBase<Ts...>;
		using ThisType = _VariantStorage<false, false, Ts...>;

		PEFF_FORCEINLINE void _reset() noexcept {
			if (this->_tag < BaseType::_INVALID_TAG_VALUE) {
				peff::_destroy_variant<Ts...>(this->_tag, this->_storage);
				this->_tag = BaseType::_INVALID_TAG_VALUE;
			}
		}

	public:
		PEFF_FORCEINLINE _VariantStorage(const ThisType &storage) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_copy_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
		}
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			this->_tag = BaseType::_INVALID_TAG_VALUE;
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) : BaseType(data) {
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) : BaseType(std::forward<T>(data)) {
		}
		PEFF_FORCEINLINE ~_VariantStorage() {
			_reset();
		}

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<T> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");
			_reset();
			peff::construct_at<T>(reinterpret_cast<std::decay_t<T> *>(this->_storage), data);
			this->_tag = BaseType::template _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			PEFF_ASSERT(&data != reinterpret_cast<std::decay_t<T> *>(BaseType::_storage), "Self-assignment is not allowed!");
			static_assert(BaseType::template _TAG_OF<std::decay_t<T>> < BaseType::_INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<std::decay_t<T>>, "The type must be move-constructible");
			_reset();
			peff::construct_at<std::decay_t<T>>(reinterpret_cast<std::decay_t<T> *>(this->_storage), std::forward<T>(data));
			this->_tag = BaseType::template _TAG_OF<std::decay_t<T>>;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(const ThisType &storage) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_reset();
			_copy_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&storage != this, "Self-assignment is not allowed!");
			_reset();
			_move_variant<Ts...>(storage._tag, this->_storage, storage._storage);
			this->_tag = storage._tag;
			storage._tag = BaseType::_INVALID_TAG_VALUE;
			return *this;
		}
	};

	// No need for _VariantStorage<false, true, Ts...>.

	/// @brief A placement-new based variant/ADT type, with non-duplicated cases.
	/// @warning Do not use it with duplicated cases or undefined behaviors will happen!
	/// @tparam ...Ts Types of the cases.
	template <typename... Ts>
	struct Variant {
	private:
		static_assert(!(std::disjunction_v<std::is_reference<Ts>...>), "Type of any case must not be reference");
		static_assert(sizeof...(Ts) != 0, "A variant with no possible case is not constructible");
		using FirstCaseType = typename FirstOf<Ts...>::type;
		using VariantStorage = _VariantStorage<
			_IsAllTriviallyDestructible<Ts...>::value,
			_IsAllTriviallyCopyable<Ts...>::value,
			Ts...>;
		VariantStorage _storage;

		template <typename T>
		constexpr static typename VariantStorage::TagType _CASE_INDEX = VariantStorage::template _TAG_OF<T>;

	public:
		template <typename T>
		constexpr static VariantIndex CASE_INDEX = (_CASE_INDEX<T> >= VariantStorage::_INVALID_TAG_VALUE ? SIZE_MAX : static_cast<VariantIndex>(_CASE_INDEX<T>));

		Variant(const Variant<Ts...> &) = default;
		PEFF_FORCEINLINE Variant(Variant<Ts...> &&rhs) noexcept(_IsAllNoexceptMoveable<Ts...>::value) : _storage(std::move(rhs._storage)) {
		}
		/// @brief Construct an instance holding a value-initialized instance of the first case.
		PEFF_FORCEINLINE Variant() noexcept(std::is_nothrow_move_constructible_v<FirstCaseType>) : _storage(FirstCaseType()) {
			static_assert(_OccurrenceTimes<0, FirstCaseType, Ts...>::value < 2, "Specified type occurred more than once");
		}
		/// @brief Construct an instance holding a copy of @p data.
		/// @param data The value to be copied into the instance.
		template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant<Ts...>>>>
		PEFF_FORCEINLINE Variant(const T &data) : _storage(data) {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
		}
		template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant<Ts...>>>>
		PEFF_FORCEINLINE Variant(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) : _storage(std::forward<T>(data)) {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
		}

		~Variant() = default;

		Variant &operator=(const Variant<Ts...> &) = default;
		PEFF_FORCEINLINE Variant &operator=(Variant<Ts...> &&rhs) noexcept(_IsAllNoexceptMoveable<Ts...>::value) {
			PEFF_ASSERT(&rhs != this, "Self-assignment is not allowed!");
			_storage = std::move(rhs._storage);
			return *this;
		}
		template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant<Ts...>>>>
		PEFF_FORCEINLINE Variant &operator=(const T &data) {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
			_storage = data;
			return *this;
		}
		template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant<Ts...>>>>
		PEFF_FORCEINLINE Variant &operator=(T &&data) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
			_storage = std::forward<T>(data);
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &get() & noexcept {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
			return _storage.get<T>();
		}

		template <typename T>
		PEFF_FORCEINLINE const T &get() const & noexcept {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
			return _storage.get<T>();
		}

		template <typename T>
		PEFF_FORCEINLINE T get() && noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
			static_assert(_OccurrenceTimes<0, T, Ts...>::value < 2, "Specified type occurred more than once");
			return std::move(_storage).get<T>();
		}

		PEFF_FORCEINLINE VariantIndex index() const noexcept {
			VariantIndex index = static_cast<VariantIndex>(_storage.get_tag());
			return index == VariantStorage::_INVALID_TAG_VALUE ? INVALID_VARIANT_INDEX : index;
		}
	};
}

#endif
