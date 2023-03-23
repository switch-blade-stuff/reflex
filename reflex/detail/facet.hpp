/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include <stdexcept>
#include <tuple>

#include "../const_string.hpp"
#include "any.hpp"

namespace reflex
{
	namespace detail
	{
		template<typename, auto>
		struct is_vtable_func : std::false_type {};
		template<typename Vtable, typename Func, Func *Vtable::*F>
		struct is_vtable_func<Vtable, F> : std::is_function<Func> {};

		template<auto>
		struct vtable_func_type;
		template<typename Vtable, typename Func, Func *Vtable::*F>
		struct vtable_func_type<F> { using type = Func; };
		template<auto F>
		using vtable_func_type_t = typename vtable_func_type<F>::type;

		template<typename>
		struct is_facet_group_impl : std::false_type {};
		template<typename... Fs>
		struct is_facet_group_impl<facet_group<Fs...>> : std::true_type {};

		/* Common type used to exploit diamond inheritance to only utilize 1 instance of `any` for group facets.
		 * Value is stored as a union since constructors of facet groups must only initialize the value once. */
		struct facet_instance
		{
			constexpr facet_instance() {}
			~facet_instance() { destroy(); }

			template<typename... Args>
			constexpr facet_instance(std::in_place_t, Args &&...args) { construct(std::forward<Args>(args)...); }

			template<typename... Args>
			constexpr void construct(Args &&...args) { std::construct_at(&value, std::forward<Args>(args)...); }
			void destroy() { std::destroy_at(&value); }

			union { any value; };
		};

		template<auto F, basic_const_string FuncName>
		[[nodiscard]] inline static bad_facet_function make_facet_error();
	}

	/** Exception type thrown when a function cannot be invoked on a facet. */
	class REFLEX_PUBLIC bad_facet_function : public std::runtime_error
	{
		template<auto F, basic_const_string FuncName>
		friend inline bad_facet_function detail::make_facet_error();

		using std::runtime_error::runtime_error;

	public:
		bad_facet_function(const bad_facet_function &) = default;
		bad_facet_function &operator=(const bad_facet_function &) = default;
		bad_facet_function(bad_facet_function &&) = default;
		bad_facet_function &operator=(bad_facet_function &&) = default;

		/** Initializes the facet error exception from message and function name strings. */
		bad_facet_function(const char *msg, std::string_view name) : std::runtime_error(msg), m_name(name) {}
		/** @copydoc bad_facet_function */
		bad_facet_function(const std::string &msg, std::string_view name) : std::runtime_error(msg), m_name(name) {}

		~bad_facet_function() override = default;

		/** Returns name of the offending facet function. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }

	private:
		std::string_view m_name;
	};

	template<auto F, basic_const_string FuncName>
	[[nodiscard]] bad_facet_function detail::make_facet_error()
	{
		constexpr auto signature = type_name<detail::vtable_func_type_t<F>>::value;
		constexpr auto msg_prefix = basic_const_string{"Failed to invoke facet function `"} + const_string<signature.size()>{signature};
		if constexpr (FuncName.empty())
		{
			const auto msg = auto_constant<msg_prefix + basic_const_string{": "} + FuncName + basic_const_string{"`"}>::value.data();
			return bad_facet_function{msg, FuncName};
		}
		else
		{
			const auto msg = auto_constant<msg_prefix + basic_const_string{"`"}>::value.data();
			return bad_facet_function{msg, FuncName};
		}
	}

	/** Utility trait used to check if type \a T is a facet type. */
	template<typename T>
	using is_facet = std::is_base_of<detail::facet_instance, T>;
	/** Alias for `is_facet<T>::value`. */
	template<typename T>
	inline constexpr auto is_facet_v = is_facet<T>::value;

	/** Utility trait used to check if a type is a facet group type. */
	template<typename T>
	struct is_facet_group : detail::is_facet_group_impl<T> {};
	/** Alias for `is_facet_group<T>::value`. */
	template<typename T>
	inline constexpr auto is_facet_group_v = is_facet_group<T>::value;

	/** Common base type for facets.
	 * @tparam Vtable Structure containing function pointers of this facet. */
	template<typename Vtable>
	class facet : detail::facet_instance
	{
		template<typename...>
		friend
		class facet_group;

	public:
		using vtable_type = Vtable;

	private:
		constexpr facet(const Vtable *vtable) noexcept : m_vtable(vtable) {}

	public:
		facet() = delete;

		facet(const facet &) noexcept = default;
		facet &operator=(const facet &) noexcept = default;
		constexpr facet(facet &&) noexcept = default;
		constexpr facet &operator=(facet &&) noexcept = default;

		/** Initializes the facet from a pointer to it's underlying vtable and an instance `any` using copy-construction.
		 * @param instance `any` containing the instance (or reference thereof) of the underlying object.
		 * @param vtable Vtable containing function pointers of the facet. */
		facet(const any &instance, const Vtable *vtable) : detail::facet_instance{std::in_place, instance}, m_vtable(vtable) {}
		/** Initializes the facet from a pointer to it's underlying vtable and an instance `any` using move-construction.
		 * @param instance `any` containing the instance (or reference thereof) of the underlying object.
		 * @param vtable Vtable containing function pointers of the facet. */
		constexpr facet(any &&instance, const Vtable *vtable) : detail::facet_instance{std::in_place, std::forward<any>(instance)}, m_vtable(vtable) {}

		/** Checks if the vtable function \a F is bound for this facet. */
		template<auto F>
		[[nodiscard]] constexpr bool is_bound() const noexcept requires std::is_member_object_pointer_v<decltype(F)>
		{
			if constexpr (detail::is_vtable_func<vtable_type, F>::value)
				return m_vtable != nullptr && vtable()->*F != nullptr;
			else
				return false;
		}

		/** Returns reference to the underlying instance `any`. */
		[[nodiscard]] constexpr any &instance() noexcept { return detail::facet_instance::value; }
		/** Returns const reference to the underlying instance `any`. */
		[[nodiscard]] constexpr const any &instance() const noexcept { return detail::facet_instance::value; }
		/** Returns pointer to the underlying vtable. */
		[[nodiscard]] constexpr const vtable_type *vtable() const noexcept { return m_vtable; }

		/** Invokes vtable function \a F with arguments \a args. Preforms additional safety checks.
		 * @tparam F Pointer to member function pointer of the underlying vtable.
		 * @tparam FuncName Function name string used for diagnostics.
		 * @param args Arguments passed to the function. */
		template<auto F, basic_const_string FuncName = "", typename... Args>
		[[nodiscard]] constexpr decltype(auto) checked_invoke(Args &&...args) const requires (requires(const vtable_type &vt) { (vt.*F)(std::forward<Args>(args)...); })
		{
			if (!is_bound<F>()) [[unlikely]] throw detail::make_facet_error<F, FuncName>();
			return (vtable()->*F)(std::forward<Args>(args)...);
		}

	private:
		const Vtable *m_vtable;
	};

	/** Utility type used to group multiple facets into a single facet type. */
	template<typename... Fs>
	class facet_group : public Fs ...
	{
		static_assert((is_facet_v<Fs> && ...), "All elements of Fs... must be facet types");
		static_assert(((!std::is_final_v<Fs>) && ...), "Grouped facet types can not be final");
		static_assert(is_unique_v<Fs...>, "Grouped facet types must not repeat");

		using vtable_type = std::tuple<const typename Fs::vtable_type *...>;

		template<typename F>
		static constexpr decltype(auto) get_vtable(auto &&vt) noexcept { return std::get<const typename F::vtable_type *>(std::forward<decltype(vt)>(vt)); }

	public:
		facet_group() = delete;

		facet_group(const facet_group &) noexcept = default;
		facet_group &operator=(const facet_group &) noexcept = default;
		constexpr facet_group(facet_group &&) noexcept = default;
		constexpr facet_group &operator=(facet_group &&) noexcept = default;

		/** Initializes the facet group from a tuple of pointers to it's underlying vtables and an instance `any` using copy-construction.
		 * @param instance `any` containing the instance (or reference thereof) of the underlying object.
		 * @param vtable Tuple of pointers to facet vtables. */
		facet_group(const any &instance, vtable_type vtable) : Fs{get_vtable<Fs>(vtable)}... { detail::facet_instance::construct(instance); }
		/** Initializes the facet group from a tuple of pointers to it's underlying vtables and an instance `any` using move-construction.
		 * @param instance `any` containing the instance (or reference thereof) of the underlying object.
		 * @param vtable Tuple of pointers to facet vtables. */
		constexpr facet_group(any &&instance, vtable_type vtable) : Fs{get_vtable<Fs>(vtable)}... { detail::facet_instance::construct(std::forward<any>(instance)); }

		/** Checks if the vtable function \a F is bound for a facet in this facet group. */
		template<auto F>
		[[nodiscard]] constexpr bool is_bound() const noexcept requires std::is_member_object_pointer_v<decltype(F)> { return (Fs::template is_bound<F>() || ...); }

		/** Returns reference to the underlying instance `any`. */
		[[nodiscard]] constexpr any &instance() noexcept { return detail::facet_instance::value; }
		/** Returns const reference to the underlying instance `any`. */
		[[nodiscard]] constexpr const any &instance() const noexcept { return detail::facet_instance::value; }
		/** Returns tuple of pointers to vtables of the grouped facets. */
		[[nodiscard]] constexpr vtable_type vtable() const noexcept { return std::forward_as_tuple(Fs::vtable()...); }
		/** Returns pointer to the underlying vtable of facet \a F. */
		template<typename F>
		[[nodiscard]] constexpr const typename F::vtable_type *vtable() const noexcept { return get_vtable<F>(vtable()); }
	};
}