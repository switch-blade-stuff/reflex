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
			constexpr auto off = 0;
#elif defined(__clang__) || defined(__GNUC__)
			constexpr auto first = Name.find('=');
			constexpr auto last = Name.rfind(']');
			constexpr auto off = 1;
#else
			constexpr auto first = decltype(Name)::npos;
			constexpr auto last = decltype(Name)::npos;
			constexpr auto off = 0;
#endif
			/* Strip slack, whitespace and keywords from the type name. */
			if constexpr (first != decltype(Name)::npos && last != decltype(Name)::npos)
				return format_type_name<const_string<last - first + off>{Name.data() + first + off + 1, Name.data() + last}>();
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
		[[nodiscard]] constexpr std::basic_string_view<char, std::char_traits<char>> make_type_name() noexcept
		{
			return auto_constant<format_type_name<REFLEX_PRETTY_FUNC>()>::value;
		}
	}

	template<typename T>
	struct type_name
	{
		using value_type = std::string_view;
		constexpr static value_type value = detail::make_type_name<T>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};
#endif
}