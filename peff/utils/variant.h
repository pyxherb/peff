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
		std::conjunction<std::is_trivially_destructible<Ts>...>;

	using VariantIndex = size_t;
	constexpr VariantIndex INVALID_VARIANT_INDEX = SIZE_MAX;

	template <bool TriviallyDestructible, bool TriviallyCopyable, typename... Ts>
	struct _VariantStorage {};

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
		void *src,
		std::index_sequence<Indices...>) {
		return (... || (index == Indices &&
						   (peff::construct_at(
								reinterpret_cast<
									std::tuple_element_t<Indices, Tuple> *>(buffer),
								*std::launder(
									reinterpret_cast<
										std::tuple_element_t<Indices, Tuple> *>(src))),
							   true)));
	}

	template <class... Ts>
	PEFF_FORCEINLINE bool _copy_variant(
		std::size_t index,
		void *buffer,
		void *src) {
		return peff::_move_variant_impl<std::tuple<Ts...>>(
			index,
			buffer,
			src,
			std::index_sequence_for<Ts...>{});
	}

	template <typename... Ts>
	struct _VariantStorageBase {
	public:
		using TagType = AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

	protected:
		char _storage[MaxSizeOf<Ts...>::value] alignas(MaxAlignOf<Ts...>::value);
		TagType _tag;

	public:
		static_assert(std::is_trivial_v<_VariantStorageBase>);
	};

	template <typename... Ts>
	struct _VariantStorage<true, false, Ts...> {
	public:
		using TagType = AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

		template <typename T>
		constexpr static TagType _TAG_OF = _VariantTagOf<0, T, Ts...>::value;

	private:
		using ThisType = _VariantStorage<true, false, Ts...>;
		char _storage[MaxSizeOf<Ts...>::value] alignas(MaxAlignOf<Ts...>::value);
		TagType _tag;

	public:
		PEFF_FORCEINLINE _VariantStorage(const ThisType &storage) noexcept {
			_copy_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
		}
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
		}
		~_VariantStorage() = default;

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
			return *this;
		}

		PEFF_FORCEINLINE ThisType &operator=(const ThisType &storage) noexcept {
			_copy_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &value() & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE const T &value() const & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<const T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE T &&value() && noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			_tag = _INVALID_TAG_VALUE;
			return std::move(reinterpret_cast<T *>(_storage));
		}

		PEFF_FORCEINLINE TagType get_tag() const noexcept {
			return _tag;
		}
	};

	template <typename... Ts>
	struct _VariantStorage<true, true, Ts...> {
	public:
		using TagType = AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

		template <typename T>
		constexpr static TagType _TAG_OF = _VariantTagOf<0, T, Ts...>::value;

	private:
		using ThisType = _VariantStorage<true, true, Ts...>;
		char _storage[MaxSizeOf<Ts...>::value] alignas(MaxAlignOf<Ts...>::value);
		TagType _tag;

	public:
		_VariantStorage(const ThisType &storage) noexcept = default;
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
		}
		~_VariantStorage() = default;

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
			return *this;
		}

		ThisType &operator=(const ThisType &storage) noexcept = default;
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &value() & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE const T &value() const & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<const T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE T &&value() && noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			_tag = _INVALID_TAG_VALUE;
			return std::move(reinterpret_cast<T *>(_storage));
		}

		PEFF_FORCEINLINE TagType get_tag() const noexcept {
			return _tag;
		}
	};

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
	struct _VariantStorage<false, false, Ts...> {
	public:
		using TagType = AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

		template <typename T>
		constexpr static TagType _TAG_OF = static_cast<TagType>(_VariantTagOf<0, T, Ts...>::value);

	private:
		using ThisType = _VariantStorage<false, false, Ts...>;
		char _storage[MaxSizeOf<Ts...>::value] alignas(MaxAlignOf<Ts...>::value);
		TagType _tag;

		PEFF_FORCEINLINE void _reset() noexcept {
			if (_tag < _INVALID_TAG_VALUE) {
				peff::_destroy_variant<Ts...>(_tag, _storage);
				_tag = _INVALID_TAG_VALUE;
			}
		}

	public:
		PEFF_FORCEINLINE _VariantStorage(const ThisType &storage) noexcept {
			_copy_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
		}
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
		}
		PEFF_FORCEINLINE ~_VariantStorage() {
			_reset();
		}

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(const ThisType &storage) noexcept {
			_copy_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			return *this;
		}
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &value() & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE const T &value() const & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<const T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE T &&value() && noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			_tag = _INVALID_TAG_VALUE;
			return std::move(reinterpret_cast<T *>(_storage));
		}

		PEFF_FORCEINLINE TagType get_tag() const noexcept {
			return _tag;
		}
	};

	template <typename... Ts>
	struct _VariantStorage<false, true, Ts...> {
	public:
		using TagType = AutoSizeUInteger<sizeof...(Ts) + 1>::type;
		static_assert(sizeof(TagType) <= sizeof(size_t), "Too many elements to the variant");
		constexpr static TagType _INVALID_TAG_VALUE = static_cast<TagType>(sizeof...(Ts));

		template <typename T>
		constexpr static TagType _TAG_OF = _VariantTagOf<0, T, Ts...>::value;

	private:
		using ThisType = _VariantStorage<false, true, Ts...>;
		char _storage[MaxSizeOf<Ts...>::value] alignas(MaxAlignOf<Ts...>::value);
		TagType _tag;

		PEFF_FORCEINLINE void _reset() noexcept {
			if (_tag < _INVALID_TAG_VALUE) {
				peff::_destroy_variant<Ts...>(_tag, _storage);
				_tag = _INVALID_TAG_VALUE;
			}
		}

	public:
		_VariantStorage(const ThisType &storage) noexcept = default;
		PEFF_FORCEINLINE _VariantStorage(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
		}
		template <typename T>
		PEFF_FORCEINLINE _VariantStorage(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
		}
		PEFF_FORCEINLINE ~_VariantStorage() {
			_reset();
		}

		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(const T &data) {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_copy_constructible_v<T>, "The type must be copy-constructible");

			peff::construct_at<T>(reinterpret_cast<T *>(_storage), data);
			_tag = _TAG_OF<T>;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE ThisType &operator=(T &&data) noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			static_assert(std::is_move_constructible_v<T>, "The type must be move-constructible");
			peff::construct_at<T>(reinterpret_cast<T *>(_storage), std::move(data));
			_tag = _TAG_OF<T>;
			return *this;
		}
		ThisType &operator=(const ThisType &storage) noexcept = default;
		PEFF_FORCEINLINE ThisType &operator=(ThisType &&storage) noexcept {
			_move_variant<Ts...>(_tag, _storage, storage._storage);
			_tag = storage._tag;
			storage._tag = _INVALID_TAG_VALUE;
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &value() & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE const T &value() const & noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			return *reinterpret_cast<const T *>(_storage);
		}

		template <typename T>
		PEFF_FORCEINLINE T &&value() && noexcept {
			static_assert(_TAG_OF<T> < _INVALID_TAG_VALUE, "The type is not in the variant");
			PEFF_ASSERT((_tag == _TAG_OF<T>), "The value stored in the variant does not match the desired type!");
			_tag = _INVALID_TAG_VALUE;
			return std::move(reinterpret_cast<T *>(_storage));
		}

		PEFF_FORCEINLINE TagType get_tag() const noexcept {
			return _tag;
		}
	};

	template <typename... Ts>
	struct Variant {
	private:
		static_assert(sizeof...(Ts) != 0, "A variant with no possible case is not constructible");
		using VariantStorage = _VariantStorage<
			_IsAllTriviallyDestructible<Ts...>::value,
			_IsAllTriviallyCopyable<Ts...>::value,
			Ts...>;
		VariantStorage _storage;

		template <typename T>
		constexpr static VariantStorage::TagType _CASE_INDEX = _VariantTagOf<0, T, Ts...>::value;

	public:
		template <typename T>
		constexpr static VariantIndex CASE_INDEX = (_CASE_INDEX<T> >= VariantStorage::_INVALID_TAG_VALUE ? SIZE_MAX : static_cast<VariantIndex>(_CASE_INDEX<T>));

		Variant(const Variant &) = default;
		PEFF_FORCEINLINE Variant(Variant &&rhs) noexcept : _storage(std::move(rhs._storage)) {
		}
		template <typename T = std::monostate>
		PEFF_FORCEINLINE Variant(const T &data = {}) : _storage(data) {
		}
		template <typename T>
		PEFF_FORCEINLINE Variant(T &&data) noexcept : _storage(std::move(data)) {
		}

		~Variant() = default;

		Variant &operator=(const Variant &) = default;
		PEFF_FORCEINLINE Variant &operator=(Variant &&rhs) {
			_storage = std::move(rhs._storage);
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE Variant &operator=(const T &data) {
			_storage = data;
			return *this;
		}
		template <typename T>
		PEFF_FORCEINLINE Variant &operator=(T &&data) noexcept {
			_storage = std::move(data);
			return *this;
		}

		template <typename T>
		PEFF_FORCEINLINE T &value() & noexcept {
			return _storage.value<T>();
		}

		template <typename T>
		PEFF_FORCEINLINE const T &value() const & noexcept {
			return _storage.value<T>();
		}

		template <typename T>
		PEFF_FORCEINLINE T &&value() && noexcept {
			return std::move(_storage).value<T>();
		}

		PEFF_FORCEINLINE VariantIndex get_index() const noexcept {
			return static_cast<VariantIndex>(_storage.get_tag());
		}
	};
}

#endif
