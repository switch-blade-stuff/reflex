/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include "detail/static_string.hpp"
#include "detail/utility.hpp"

namespace REFLEX_NAMESPACE
{
	/** Helper type used to obtain name of a type. Serves as a user customization point.
	 * @note When not overloaded, type name is generated using a compiler-specific method.
	 * Is no overload is available, `type_name` has no default overload. */
	template<typename T>
	struct type_name;

#ifdef REFLEX_PRETTY_FUNC
	namespace detail
	{
		template<basic_static_string Name>
		[[nodiscard]] constexpr auto format_type_name() noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto offset_start = Name.find('=') + 2;
			constexpr auto offset_end = Name.rfind(']');
#elif defined(_MSC_VER)
			constexpr auto offset_start = Name.find('<') + 1;
			constexpr auto offset_end = Name.rfind('>');
#else
			constexpr auto offset_start = 0;
			constexpr auto offset_end = Name.size();
#endif
			using result_t = static_string<offset_end - offset_start + 1>;
			return result_t{Name.begin() + offset_start, Name.begin() + offset_end};
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
		constexpr static value_type value = detail::make_type_name<T>();

		[[nodiscard]] constexpr operator value_type() noexcept { return value; }
		[[nodiscard]] constexpr value_type operator()() noexcept { return value; }
	};
#endif
}