/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <vector>

#include "fwd.hpp"

namespace reflex
{
	namespace detail
	{
		enum class argument_flags : int
		{
			/** Argument is const-qualified. */
			is_const = type_flags::is_const,
			/** Argument is passed by value. */
			is_value = type_flags::is_value
		};

		constexpr argument_flags operator~(const argument_flags &x) noexcept { return static_cast<argument_flags>(~static_cast<int>(x)); }
		constexpr argument_flags operator&(const argument_flags &a, const argument_flags &b) noexcept { return static_cast<argument_flags>(static_cast<int>(a) & static_cast<int>(b)); }
		constexpr argument_flags operator|(const argument_flags &a, const argument_flags &b) noexcept { return static_cast<argument_flags>(static_cast<int>(a) | static_cast<int>(b)); }
		constexpr argument_flags operator^(const argument_flags &a, const argument_flags &b) noexcept { return static_cast<argument_flags>(static_cast<int>(a) ^ static_cast<int>(b)); }
		constexpr argument_flags &operator&=(argument_flags &a, int b) noexcept { return a = static_cast<argument_flags>(static_cast<int>(a) & static_cast<int>(b)); }
		constexpr argument_flags &operator|=(argument_flags &a, int b) noexcept { return a = static_cast<argument_flags>(static_cast<int>(a) | static_cast<int>(b)); }
		constexpr argument_flags &operator^=(argument_flags &a, int b) noexcept { return a = static_cast<argument_flags>(static_cast<int>(a) ^ static_cast<int>(b)); }
	}

	/** Structure representing a sequence of arguments. */
	class argument_list
	{
		friend class constructor_info;

		template<typename... Args>
		inline argument_list(type_pack_t<Args...>, detail::database_impl *);
		inline argument_list(std::span<const detail::arg_data> data, detail::database_impl *db);

	public:
		using value_type = argument_info;
		using reference = argument_info;

		class pointer;
		class iterator;

		using difference_type = std::ptrdiff_t;
		using size_type = std::size_t;

	public:
		constexpr argument_list() noexcept = default;
		constexpr argument_list(const argument_list &) noexcept = default;
		constexpr argument_list(argument_list &&) noexcept = default;
		argument_list &operator=(const argument_list &) noexcept = default;
		argument_list &operator=(argument_list &&) noexcept = default;

		/** Creates argument list for argument types \a Args. */
		template<typename... Args>
		inline argument_list(type_pack_t<Args...>);
		/** Creates argument list from initializer list of pairs \a args where `first` is the type of the argument and `second` is the flags bitmask. */
		inline argument_list(std::initializer_list<std::pair<type_info, detail::argument_flags>> args);

		/** Checks if the argument list is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept;
		/** Returns total amount of arguments in the argument list. */
		[[nodiscard]] constexpr size_type size() const noexcept;

		/** Returns iterator to the first argument of the list. */
		[[nodiscard]] constexpr iterator begin() const noexcept;
		/** @copydoc begin */
		[[nodiscard]] constexpr iterator cbegin() const noexcept;
		/** Returns iterator one past the last argument of the list. */
		[[nodiscard]] constexpr iterator end() const noexcept;
		/** @copydoc begin */
		[[nodiscard]] constexpr iterator cend() const noexcept;

		/** Returns the first argument of the argument list. */
		[[nodiscard]] constexpr value_type front() const noexcept;
		/** Returns the last argument of the argument list. */
		[[nodiscard]] constexpr value_type back() const noexcept;
		/** Returns the argument located at index \a i in the argument list. */
		[[nodiscard]] constexpr value_type operator[](size_type i) const noexcept;

	private:
		std::vector<detail::arg_data> m_data;
		detail::database_impl *m_db = nullptr;
	};
	/** Structure containing information about an argument of an argument list. */
	class argument_info
	{
		friend class argument_list::iterator;

		constexpr argument_info(const detail::arg_data *data, detail::database_impl *db) noexcept : m_data(data), m_db(db) {}

	public:
		using flags_type = detail::argument_flags;

	public:
		argument_info() = delete;

		constexpr argument_info(const argument_info &) noexcept = default;
		constexpr argument_info(argument_info &&) noexcept = default;
		constexpr argument_info &operator=(const argument_info &) noexcept = default;
		constexpr argument_info &operator=(argument_info &&) noexcept = default;

		/** Returns flags of the argument info. */
		[[nodiscard]] constexpr flags_type flags() const noexcept;

		/** Checks if the argument type is a reference. */
		[[nodiscard]] inline bool is_ref() const noexcept;
		/** Checks if the argument type is const-qualified. */
		[[nodiscard]] inline bool is_const() const noexcept;
		/** Returns type of the argument. */
		[[nodiscard]] inline type_info type() const noexcept;

		/** @warning Internal use only! */
		[[nodiscard]] constexpr operator const detail::arg_data &() const noexcept { return *m_data; }

		[[nodiscard]] constexpr bool operator==(const argument_info &other) const noexcept;

	private:
		const detail::arg_data *m_data;
		detail::database_impl *m_db;
	};

	/** Structure representing a view of type constructors. */
	class constructor_view
	{
		friend class type_info;

		constexpr constructor_view(const std::list<detail::type_ctor> *list, detail::database_impl *db) : m_view(list), m_db(db) {}

	public:
		using value_type = constructor_info;
		using reference = constructor_info;
		using size_type = std::size_t;

		class pointer;
		class iterator;

	public:
		constexpr constructor_view() noexcept = default;
		constexpr constructor_view(const constructor_view &) noexcept = default;
		constexpr constructor_view(constructor_view &&) noexcept = default;
		constructor_view &operator=(const constructor_view &) noexcept = default;
		constructor_view &operator=(constructor_view &&) noexcept = default;

		/** Checks if the constructor list is empty. */
		[[nodiscard]] bool empty() const noexcept { return m_view->empty(); }
		/** Returns total amount of constructors in the constructor list. */
		[[nodiscard]] size_type size() const noexcept { return m_view->size(); }

		/** Returns iterator to the first argument of the list. */
		[[nodiscard]] inline iterator begin() const noexcept;
		/** @copydoc begin */
		[[nodiscard]] inline iterator cbegin() const noexcept;
		/** Returns iterator one past the last argument of the list. */
		[[nodiscard]] inline iterator end() const noexcept;
		/** @copydoc begin */
		[[nodiscard]] inline iterator cend() const noexcept;

	private:
		const std::list<detail::type_ctor> *m_view = nullptr;
		detail::database_impl *m_db = nullptr;
	};
	/** Structure representing constructor of a reflected type. */
	class constructor_info
	{
		friend class constructor_view::iterator;

		constexpr constructor_info(const detail::type_ctor *data, detail::database_impl *db) : m_data(data), m_db(db) {}

	public:
		constructor_info() = delete;

		constexpr constructor_info(const constructor_info &) noexcept = default;
		constexpr constructor_info(constructor_info &&) noexcept = default;
		constexpr constructor_info &operator=(const constructor_info &) noexcept = default;
		constexpr constructor_info &operator=(constructor_info &&) noexcept = default;

		/** Returns argument list of the referenced constructor. */
		[[nodiscard]] inline argument_list args() const noexcept;

		/** Checks if the underlying constructor can be invoked with arguments \a args. */
		[[nodiscard]] REFLEX_PUBLIC bool is_invocable(argument_list args) const;
		/** @copydoc is_invocable */
		[[nodiscard]] REFLEX_PUBLIC bool is_invocable(std::span<any> args) const;

		/** Invokes the underlying constructor with arguments \a args. */
		[[nodiscard]] inline any invoke(std::span<any> args) const;
		/** @copydoc invoke */
		[[nodiscard]] inline any operator()(std::span<any> args) const;

		[[nodiscard]] constexpr bool operator==(const constructor_info &other) const noexcept;

	private:
		const detail::type_ctor *m_data = nullptr;
		detail::database_impl *m_db = nullptr;
	};

	/** Handle to reflected type information. */
	class type_info
	{
		template<typename>
		friend class type_factory;
		template<typename...>
		friend class type_query;

		friend class argument_list;
		friend class argument_info;
		friend class any;

	public:
		/** Returns type query used to filter reflected types. */
		[[nodiscard]] inline static type_query<> query();

		/** Reflects type info for type \a T and returns a type factory used for initialization.
		 * @note Modifications of type info made through the type factory are not thread-safe and must be synchronized externally. */
		template<typename T>
		inline static type_factory<T> reflect();

		/** Returns type info for type with name \a name, or an invalid type info if the type has not been reflected yet. */
		[[nodiscard]] inline static type_info get(std::string_view name);
		/** Returns type info for type \a T. */
		template<typename T>
		[[nodiscard]] inline static type_info get();

		/** Resets type info for type with name \a name to it's initial state. */
		inline static void reset(std::string_view name);
		/** Resets type info for type \a T to it's initial state. */
		template<typename T>
		inline static void reset();
		/** Resets all reflected `type_info`s to their initial state. */
		inline static void reset();

	private:
		type_info(detail::type_handle handle, detail::database_impl &db) : m_data(handle(db)), m_db(&db) {}
		constexpr type_info(detail::type_data *data, detail::database_impl *db) noexcept : m_data(data), m_db(db) {}

	public:
		/** Initializes an invalid type info. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns the name of the referenced type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept;

		/** Returns the size of the referenced type.
		 * @note If `is_empty_v<T>` evaluates to `true` for referenced type \a T, returns `0` instead of `1`. */
		[[nodiscard]] constexpr std::size_t size() const noexcept;
		/** Returns the alignment of the referenced type. */
		[[nodiscard]] constexpr std::size_t alignment() const noexcept;

		/** Checks if the referenced type is `void`. */
		[[nodiscard]] constexpr bool is_void() const noexcept;
		/** Checks if the referenced type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept;
		/** Checks if the referenced type is `std::nullptr_t`. */
		[[nodiscard]] constexpr bool is_nullptr() const noexcept;

		/** Checks if the referenced type is an enum. */
		[[nodiscard]] constexpr bool is_enum() const noexcept;
		/** Checks if the referenced type is a class. */
		[[nodiscard]] constexpr bool is_class() const noexcept;
		/** Checks if the referenced type is an abstract type. */
		[[nodiscard]] constexpr bool is_abstract() const noexcept;

		/** Checks if the referenced type is a pointer type.
		 * @note For pointer-like class types, check the `facets::pointer` facet. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept;
		/** Checks if the referenced type is an integral type.
		 * @note For conversions to integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_integral() const noexcept;
		/** Checks if the referenced type is a signed integral type.
		 * @note For conversions to signed integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_signed_integral() const noexcept;
		/** Checks if the referenced type is an unsigned integral type.
		 * @note For conversions to unsigned integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_unsigned_integral() const noexcept;
		/** Checks if the referenced type is an arithmetic type.
		 * @note For conversions to arithmetic types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_arithmetic() const noexcept;

		/** Removes extent from the referenced type. */
		[[nodiscard]] inline type_info remove_extent() const noexcept;
		/** Removes pointer from the referenced type. */
		[[nodiscard]] inline type_info remove_pointer() const noexcept;

		/** Checks if the referenced type is an array type. */
		[[nodiscard]] constexpr bool is_array() const noexcept;
		/** Returns the extent of the referenced type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept;

		/** Checks if the referenced type has an attribute of type \a T. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept { return has_attribute(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type has an attribute of type \a type. */
		[[nodiscard]] bool has_attribute(type_info type) const noexcept { return type.valid() && has_attribute(type.name()); }

		/** Returns a map of the referenced type's attributes. */
		[[nodiscard]] REFLEX_PUBLIC detail::attr_map attributes() const;
		/** Returns value of the attribute of type \a T, or an empty `any` if no such attribute exists. */
		template<typename T>
		[[nodiscard]] inline any attribute() const;
		/** Returns value of the attribute of type \a type, or an empty `any` if no such attribute exists. */
		[[nodiscard]] inline any attribute(type_info type) const;

		/** Checks if the referenced type has an enumeration with value \a value. */
		template<typename T>
		[[nodiscard]] inline bool has_enumeration(T &&value) const requires (!std::same_as<std::decay_t<T>, any>);
		/** @copydoc has_enumeration */
		[[nodiscard]] REFLEX_PUBLIC bool has_enumeration(const any &value) const;
		/** Checks if the referenced type has an enumeration with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool has_enumeration(std::string_view name) const noexcept;

		/** Returns a map of the referenced type's enumerations. */
		[[nodiscard]] REFLEX_PUBLIC detail::enum_map enumerations() const;
		/** Returns value of the attribute of type with name \a name, or an empty `any` if no such attribute exists. */
		[[nodiscard]] REFLEX_PUBLIC any attribute(std::string_view name) const;
		/** Returns value of the enumeration with name \a name, or an empty `any` if no such enumeration exists. */
		[[nodiscard]] REFLEX_PUBLIC any enumerate(std::string_view name) const;

		/** Checks if the referenced type implements all facets in facet group \a G. */
		template<instance_of<facets::facet_group> G>
		[[nodiscard]] inline bool implements_facet() const;
		/** Checks if the referenced type implements a facet type \a F. */
		template<typename F>
		[[nodiscard]] inline bool implements_facet() const { return implements_facet(type_name_v<F>); }
		/** Checks if the referenced type implements a facet type \a type.
		 * @note \a type must not be type of a facet group. */
		[[nodiscard]] inline bool implements_facet(type_info type) const { return implements_facet(type.name()); }

		/** Returns facet group of type \a G for object instance \a obj. */
		template<instance_of<facets::facet_group> G>
		[[nodiscard]] inline G facet(const any &obj) const;
		/** @copydoc facet */
		template<instance_of<facets::facet_group> G>
		[[nodiscard]] inline G facet(any &&obj) const;
		/** Returns facet of type \a F for object instance \a obj. */
		template<typename F>
		[[nodiscard]] inline F facet(const any &obj) const;
		/** @copydoc facet */
		template<typename F>
		[[nodiscard]] inline F facet(any &&obj) const;

		/** Checks if the referenced type inherits from a base type \a T. */
		template<typename T>
		[[nodiscard]] bool inherits_from() const { return inherits_from(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type inherits from a base type \a type. */
		[[nodiscard]] bool inherits_from(type_info type) const { return type.valid() && inherits_from(type.name()); }

		/** Returns a set of the referenced type's parents (including the parents' parents). */
		[[nodiscard]] REFLEX_PUBLIC detail::type_set parents() const;

		/** Checks if the referenced type is constructible from arguments \a Args. */
		template<typename... Args>
		[[nodiscard]] inline bool constructible_from() const { return constructible_from(argument_list{type_pack<Args...>}); }
		/** Checks if the referenced type is constructible from arguments \a args. */
		[[nodiscard]] REFLEX_PUBLIC bool constructible_from(argument_list args) const;
		/** @copydoc constructible_from */
		[[nodiscard]] REFLEX_PUBLIC bool constructible_from(std::span<any> args) const;

		/** Returns a view of the referenced type's constructors. */
		[[nodiscard]] constexpr constructor_view constructors() const noexcept;
		/** Constructs an object of the referenced type from arguments \a args.
		 * @return `any` containing the constructed object instance, or an empty `any` if `this` is not valid or the referenced type is not constructible from \a args. */
		template<typename... Args>
		[[nodiscard]] inline any construct(Args &&...args) const;
		/** @cpoydoc construct */
		template<std::size_t N>
		[[nodiscard]] inline any construct(std::span<any, N> args) const;
		/** @cpoydoc construct */
		[[nodiscard]] REFLEX_PUBLIC any construct(std::span<any> args) const;

		/** Checks if the referenced type is convertible to type \a T, or inherits from a type convertible to \a T. */
		template<typename T>
		[[nodiscard]] bool convertible_to() const { return convertible_to(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is convertible to type \a type, or inherits from a type convertible to \a type. */
		[[nodiscard]] bool convertible_to(type_info type) const { return type.valid() && convertible_to(type.name()); }

		/** Checks if the referenced type is same as, inherits from, or can be type-cast to type \a T. */
		template<typename T>
		[[nodiscard]] bool compatible_with() const { return compatible_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is same as, inherits from, or can be type-cast to type \a type. */
		[[nodiscard]] bool compatible_with(type_info type) const { return *this == type || inherits_from(type) || convertible_to(type); }

		/** Checks if the referenced type is directly (without conversion) comparable (using any comparison operator) with type \a T. */
		template<typename T>
		[[nodiscard]] bool comparable_with() const { return comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable (using any comparison operator) with type \a type. */
		[[nodiscard]] bool comparable_with(type_info type) const noexcept { return comparable_with(type.name()); }

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator==` and `operator!=`. */
		template<typename T>
		[[nodiscard]] bool eq_comparable_with() const { return eq_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator==` and `operator!=`. */
		[[nodiscard]] bool eq_comparable_with(type_info type) const noexcept { return eq_comparable_with(type.name()); }

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator>=`. */
		template<typename T>
		[[nodiscard]] bool ge_comparable_with() const { return ge_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator>=`. */
		[[nodiscard]] bool ge_comparable_with(type_info type) const noexcept { return ge_comparable_with(type.name()); }

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator<=`. */
		template<typename T>
		[[nodiscard]] bool le_comparable_with() const { return le_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator<=`. */
		[[nodiscard]] bool le_comparable_with(type_info type) const noexcept { return le_comparable_with(type.name()); }

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator>`. */
		template<typename T>
		[[nodiscard]] bool gt_comparable_with() const { return gt_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator>`. */
		[[nodiscard]] bool gt_comparable_with(type_info type) const noexcept { return gt_comparable_with(type.name()); }

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator<`. */
		template<typename T>
		[[nodiscard]] bool lt_comparable_with() const { return lt_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator<`. */
		[[nodiscard]] bool lt_comparable_with(type_info type) const noexcept { return lt_comparable_with(type.name()); }

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;

	private:
		/* Convenience operator for access to underlying type_data. */
		[[nodiscard]] constexpr detail::type_data *operator->() const noexcept { return m_data; }

		[[nodiscard]] REFLEX_PUBLIC bool has_attribute(std::string_view name) const noexcept;

		[[nodiscard]] REFLEX_PUBLIC bool implements_facet(std::string_view name) const;
		[[nodiscard]] REFLEX_PUBLIC bool inherits_from(std::string_view name) const;
		[[nodiscard]] REFLEX_PUBLIC bool convertible_to(std::string_view name) const;

		[[nodiscard]] REFLEX_PUBLIC bool comparable_with(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool eq_comparable_with(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool ge_comparable_with(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool le_comparable_with(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool gt_comparable_with(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool lt_comparable_with(std::string_view name) const noexcept;

		[[nodiscard]] REFLEX_PUBLIC const void *get_vtab(std::string_view name) const;

		template<typename T>
		[[nodiscard]] auto get_vtab() const { return static_cast<const typename T::vtable_type *>(get_vtab(type_name_v<T>)); }
		template<typename... Ts>
		[[nodiscard]] auto get_vtab(type_pack_t<Ts...>) const { return std::forward_as_tuple(get_vtab<Ts>()...); }

		inline void fill_parents(detail::type_set &result) const;

		detail::type_data *m_data = nullptr;
		detail::database_impl *m_db = nullptr;
	};

	[[nodiscard]] constexpr bool operator==(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator==(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }
	[[nodiscard]] constexpr bool operator!=(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator!=(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }

	/** Returns the type info of \a T. Equivalent to `type_info::get<T>()`. */
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&) { return type_info::get<T>(); }

	namespace literals
	{
		/** Equivalent to `type_info::get`. */
		[[nodiscard]] inline type_info operator ""_type(const char *str, std::size_t n) { return type_info::get({str, n}); }
	}
}

template<auto F>
struct tpp::hash<reflex::type_info, F> { std::size_t operator()(const reflex::type_info &value) const { return tpp::hash<std::string_view, F>{}(value.name()); }};
template<>
struct std::hash<reflex::type_info> { std::size_t operator()(const reflex::type_info &value) const { return std::hash<std::string_view>{}(value.name()); }};
