/*
 * Created by switchblade on 2023-03-31.
 */

#include "common.hpp"

int main()
{
	constexpr auto arr_val = std::array{0, 1};
	const auto arr_ti = reflex::type_info::get<std::array<int, 2>>();

	TEST_ASSERT(arr_ti.implements_facet<reflex::facets::tuple>());
	TEST_ASSERT(arr_ti.implements_facet<reflex::facets::range>());

	const auto arr = arr_ti.construct(arr_val);
	TEST_ASSERT((!arr.empty() && arr.get<std::array<int, 2>>() == arr_val));

	const auto fr = arr.facet<reflex::facets::range>();
	const auto ft = arr.facet<reflex::facets::tuple>();

	TEST_ASSERT(!fr.empty());
	TEST_ASSERT(fr.value_type() == reflex::type_info::get<int>());
	TEST_ASSERT(ft.first_type() == fr.value_type());
	TEST_ASSERT(ft.second_type() == fr.value_type());

	TEST_ASSERT(ft.first() == fr.at(0));
	TEST_ASSERT(ft.second() == fr.at(1));
	TEST_ASSERT(ft.first() == reflex::forward_any(0));
	TEST_ASSERT(ft.second() == reflex::forward_any(1));
}