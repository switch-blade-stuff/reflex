/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include "database.hpp"

namespace reflex
{
#ifndef REFLEX_NO_ARITHMETIC
	template struct type_init<bool>;

	template struct type_init<char>;
	template struct type_init<wchar_t>;
	template struct type_init<char8_t>;
	template struct type_init<char16_t>;
	template struct type_init<char32_t>;

	template struct type_init<std::int8_t>;
	template struct type_init<std::int16_t>;
	template struct type_init<std::int32_t>;
	template struct type_init<std::int64_t>;
	template struct type_init<std::uint8_t>;
	template struct type_init<std::uint16_t>;
	template struct type_init<std::uint32_t>;
	template struct type_init<std::uint64_t>;

#if INTPTR_WIDTH != INT64_WIDTH
	template struct type_init<std::intptr_t>;
	template struct type_init<std::uintptr_t>;
#endif

#if INTMAX_WIDTH != INT64_WIDTH && INTMAX_WIDTH != INTPTR_WIDTH
	template struct type_init<std::intmax_t>;
	template struct type_init<std::uintmax_t>;
#endif

#if PTRDIFF_WIDTH != INT64_WIDTH && PTRDIFF_WIDTH != INTPTR_WIDTH && PTRDIFF_WIDTH != INTMAX_WIDTH
	template struct type_init<std::ptrdiff_t>
#endif
#if SIZE_WIDTH != UINT64_WIDTH && SIZE_WIDTH != UINTPTR_WIDTH && SIZE_WIDTH != UINTMAX_WIDTH
	template struct type_init<std::size_t>;
#endif

	template struct type_init<float>;
	template struct type_init<double>;
	template struct type_init<long double>;
#endif
}