/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include "detail/utility.hpp"
#include "const_string.hpp"

namespace reflex
{
	/** Helper type used to obtain name of a type. Serves as a user customization point.
	 * @note When not overloaded, type name is generated using a compiler-specific method.
	 * Is no overload is available, `type_name` has no default overload. */
	template<typename T>
	struct type_name;

	/** Alias for `type_name<T>::value`. */
	template<typename T>
	inline constexpr auto type_name_v = type_name<T>::value;

#ifdef REFLEX_PRETTY_FUNC
	namespace detail
	{
		template<basic_const_string Str, basic_const_string Sub, std::size_t Off = 0>
		[[nodiscard]] consteval auto erase_substr() noexcept
		{
			constexpr auto pos = Str.find(Sub) + Off;
			const_string<Str.size() - Sub.size() + Off> result;
			std::copy(Str.begin(), Str.begin() + pos, result.begin());
			std::copy(Str.begin() + Sub.size() + pos - Off, Str.end(), result.begin() + pos);
			return result;
		}

		template<basic_const_string Name>
		[[nodiscard]] constexpr auto format_type_name() noexcept
		{
#if defined(_MSC_VER)
			constexpr auto first = Name.find('<', Name.find("make_type_name"));
			constexpr auto last = Name.rfind('>');
			constexpr auto off = 1;
#elif defined(__clang__) || defined(__GNUC__)
			constexpr auto first = Name.find('=');
			constexpr auto last = Name.rfind(']');
			constexpr auto off = 2;
#else
			constexpr auto first = decltype(Name)::npos;
			constexpr auto last = decltype(Name)::npos;
			constexpr auto off = 0;
#endif
			/* Strip slack, whitespace and keywords from the type name. */
			if constexpr (first != decltype(Name)::npos && last != decltype(Name)::npos)
				return format_type_name<const_string<last - (first + off)>{Name.data() + first + off, Name.data() + last}>();
			else if constexpr (Name.contains("struct "))
				return format_type_name<erase_substr<Name, "struct ">()>();
			else if constexpr (Name.contains("union "))
				return format_type_name<erase_substr<Name, "union ">()>();
			else if constexpr (Name.contains("class "))
				return format_type_name<erase_substr<Name, "class ">()>();
			else if constexpr (Name.contains("< "))
				return format_type_name<erase_substr<Name, "< ", 1>()>();
			else if constexpr (Name.contains("> "))
				return format_type_name<erase_substr<Name, "> ", 1>()>();
			else if constexpr (Name.contains(", "))
				return format_type_name<erase_substr<Name, ", ", 1>()>();
			else
				return Name;
		}
		template<typename T>
		[[nodiscard]] constexpr std::basic_string_view<char> make_type_name() noexcept
		{
			return auto_constant<format_type_name<REFLEX_PRETTY_FUNC>()>::value;
		}
	}

	template<typename T>
	struct type_name
	{
		using value_type = std::string_view;
		static constexpr value_type value = detail::make_type_name<T>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};
#endif

	namespace detail
	{
		/* Pointer, reference, array & cv-qualified types will not respect specializations of `type_name` for their un-decorated types.
		 * As such, format such types manually. */
		template<std::integral auto I, basic_const_string Str = "">
		[[nodiscard]] constexpr auto const_itoa() noexcept
		{
			constexpr auto dec = I / decltype(I){10};
			constexpr auto rem = I % decltype(I){10};
			constexpr auto rem_str = const_string<1>{{'0' + static_cast<char>(rem)}} + Str;
			if constexpr (dec != 0)
				return const_itoa<dec, rem_str>();
			else
				return rem_str;
		}
		template<typename T>
		[[nodiscard]] constexpr auto make_array_prefix() noexcept
		{
			if constexpr (!(std::is_pointer_v<T> || std::is_reference_v<T>))
				return basic_const_string{" "};
			else
				return basic_const_string{""};
		}
		template<typename T>
		[[nodiscard]] constexpr auto make_deref_prefix() noexcept
		{
			if constexpr (std::is_reference_v<T> || (std::is_pointer_v<T> && !(std::is_const_v<T> || std::is_volatile_v<T>)))
				return basic_const_string{""};
			else
				return basic_const_string{" "};
		}
		template<typename T>
		[[nodiscard]] constexpr auto eval_qualify_type_name() noexcept
		{
			if constexpr (std::is_pointer_v<T>) /* Handle qualified pointers separately. */
			{
				using value_type = std::remove_pointer_t<T>;
				constexpr auto value_name = type_name_v<value_type>;
				constexpr auto ptr_name = const_string<value_name.size()>{value_name} + make_deref_prefix<value_type>() + basic_const_string{"*"};

				if constexpr (std::is_const_v<T>)
					return ptr_name + basic_const_string{"const"};
				else if constexpr (std::is_volatile_v<T>)
					return ptr_name + basic_const_string{"volatile"};
				else
					return ptr_name;
			}
			else if constexpr (std::is_const_v<T>)
			{
				constexpr auto value_name = type_name_v<std::remove_const_t<T>>;
				return basic_const_string{"const "} + const_string<value_name.size()>{value_name};
			}
			else if constexpr (std::is_volatile_v<T>)
			{
				constexpr auto value_name = type_name_v<std::remove_volatile_t<T>>;
				return basic_const_string{"volatile "} + const_string<value_name.size()>{value_name};
			}
			else if constexpr (std::is_array_v<std::remove_reference_t<T>>) /* Handle array references separately. */
			{
				using value_type = std::remove_extent_t<std::remove_reference_t<T>>;
				constexpr auto value_name = type_name_v<value_type>;
				constexpr auto ref = std::string_view{std::is_reference_v<T> ? (std::is_rvalue_reference_v<T> == 2 ? "(&&)" : "(&)") : ""};
				constexpr auto infix = ref.empty() ? ref : std::string_view{auto_constant<make_array_prefix<value_type>() + const_string<ref.size()>{ref}>::value};
				constexpr auto extent = std::extent_v<std::remove_reference_t<T>>;
				if constexpr (extent != 0)
					return const_string<value_name.size()>{value_name} + const_string<infix.size()>{infix} + basic_const_string{"["} + const_itoa<extent>() + basic_const_string{"]"};
				else
					return const_string<value_name.size()>{value_name} + const_string<infix.size()>{infix} + basic_const_string{"[]"};
			}
			else if constexpr (std::is_lvalue_reference_v<T>)
			{
				using value_type = std::remove_reference_t<T>;
				constexpr auto value_name = type_name_v<value_type>;
				return const_string<value_name.size()>{value_name} + make_deref_prefix<value_type>() + basic_const_string{"&"};
			}
			else if constexpr (std::is_rvalue_reference_v<T>)
			{
				using value_type = std::remove_reference_t<T>;
				constexpr auto value_name = type_name_v<value_type>;
				return const_string<value_name.size()>{value_name} + make_deref_prefix<value_type>() + basic_const_string{"&&"};
			}
			else
			{
				constexpr auto name = type_name<T>::value;
				return const_string<name.size()>{name};
			}
		}
		template<typename T>
		[[nodiscard]] constexpr std::string_view qualify_type_name() noexcept
		{
			return auto_constant<eval_qualify_type_name<T>()>::value;
		}
	}

	template<typename T> requires (std::is_pointer_v<T> || std::is_reference_v<T> || std::is_array_v<T> || std::is_const_v<T> || std::is_volatile_v<T>)
	struct type_name<T>
	{
		using value_type = std::string_view;
		static constexpr value_type value = detail::qualify_type_name<T>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};

	namespace detail
	{
		template<typename... Args>
		[[nodiscard]] constexpr auto function_args_str() noexcept
		{
			constexpr auto arg_name = []<typename U>(std::in_place_type_t<U>)
			{
				constexpr auto name = type_name_v<U>;
				return const_string<name.size()>{name};
			};
			return basic_const_string{"("} + (arg_name(std::in_place_type<Args>) + ... + basic_const_string{")"});
		}
		template<typename R, typename... Args>
		[[nodiscard]] constexpr auto eval_function_name() noexcept
		{
			constexpr auto ret = type_name<R>::value;
			return const_string<ret.size()>{ret} + function_args_str<Args...>();
		}
		template<typename R, typename... Args>
		[[nodiscard]] constexpr std::string_view function_name() noexcept
		{
			return auto_constant<eval_function_name<R, Args...>()>::value;
		}
		template<typename R, typename... Args>
		[[nodiscard]] constexpr std::string_view noexcept_function_name() noexcept
		{
			return auto_constant<eval_function_name<R, Args...>() + basic_const_string{" noexcept"}>::value;
		}
	}

	template<typename R, typename... Args>
	struct type_name<R(Args...)>
	{
		using value_type = std::string_view;
		static constexpr value_type value = detail::function_name<R, Args...>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};

	template<typename R, typename... Args>
	struct type_name<R(Args...) noexcept>
	{
		using value_type = std::string_view;
		static constexpr value_type value = detail::noexcept_function_name<R, Args...>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};
}