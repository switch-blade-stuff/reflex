/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include "database.hpp"

namespace reflex::detail
{
#ifndef REFLEX_NO_ARITHMETIC
	template<typename T>
	void init_arithmetic(type_factory<T> factory)
	{
		const auto init_metadata = [&]<typename U>(std::in_place_type_t<U>)
		{
			if constexpr (!std::same_as<T, U>)
			{
				if constexpr (std::constructible_from<T, U>)
					factory.template make_constructible<U>([](U x) { return static_cast<T>(x); });
				if constexpr (std::convertible_to<T, U>)
					factory.template make_convertible<U>();
				if constexpr (std::three_way_comparable_with<T, U>)
					factory.template make_comparable<U>();
			}
		};
		const auto init_unwrap = [&]<typename... Ts>(type_pack_t<Ts...>) { (init_metadata(std::in_place_type<Ts>), ...); };

		init_metadata(std::in_place_type<bool>);
		init_unwrap(unique_type_pack<type_pack_t<
				char, wchar_t, char8_t, char16_t, char32_t,
				std::int8_t, std::int16_t, std::int32_t, std::int64_t,
				std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
				std::intmax_t, std::uintmax_t, std::intptr_t, std::uintptr_t,
				std::ptrdiff_t, std::size_t>>);
		init_unwrap(type_pack<float, double, long double>);
	}

	template void init_arithmetic<bool>(type_factory<bool>);

	template void init_arithmetic<char>(type_factory<char>);
	template void init_arithmetic<wchar_t>(type_factory<wchar_t>);
	template void init_arithmetic<char8_t>(type_factory<char8_t>);
	template void init_arithmetic<char16_t>(type_factory<char16_t>);
	template void init_arithmetic<char32_t>(type_factory<char32_t>);

	template void init_arithmetic<std::int8_t>(type_factory<std::int8_t>);
	template void init_arithmetic<std::int16_t>(type_factory<std::int16_t>);
	template void init_arithmetic<std::int32_t>(type_factory<std::int32_t>);
	template void init_arithmetic<std::int64_t>(type_factory<std::int64_t>);
	template void init_arithmetic<std::uint8_t>(type_factory<std::uint8_t>);
	template void init_arithmetic<std::uint16_t>(type_factory<std::uint16_t>);
	template void init_arithmetic<std::uint32_t>(type_factory<std::uint32_t>);
	template void init_arithmetic<std::uint64_t>(type_factory<std::uint64_t>);

#if INTPTR_WIDTH != INT64_WIDTH
	template void init_arithmetic<std::intptr_t>(type_factory<std::intptr_t>);
	template void init_arithmetic<std::uintptr_t>(type_factory<std::uintptr_t>);
#endif

#if INTMAX_WIDTH != INT64_WIDTH && INTMAX_WIDTH != INTPTR_WIDTH
	template void init_arithmetic<std::intmax_t>(type_factory<std::intmax_t>);
	template void init_arithmetic<std::uintmax_t>(type_factory<std::uintmax_t>);
#endif

#if PTRDIFF_WIDTH != INT64_WIDTH && PTRDIFF_WIDTH != INTPTR_WIDTH && PTRDIFF_WIDTH != INTMAX_WIDTH
	template void init_arithmetic<std::ptrdiff_t>(type_factory<std::ptrdiff_t>);
#endif
#if SIZE_WIDTH != UINT64_WIDTH && SIZE_WIDTH != UINTPTR_WIDTH && SIZE_WIDTH != UINTMAX_WIDTH
	template void init_arithmetic<std::size_t>(type_factory<std::size_t>);
#endif

	template void init_arithmetic<float>(type_factory<float>);
	template void init_arithmetic<double>(type_factory<double>);
	template void init_arithmetic<long double>(type_factory<long double>);
#endif
}