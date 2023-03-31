/*
 * Created by switchblade on 2023-03-31.
 */

#include "common.hpp"

int main()
{
	constexpr auto str_val = std::string_view{"hello, world"};
	const auto str_ti = reflex::type_info::get<std::string>();

	TEST_ASSERT(str_ti.implements_facet<reflex::facets::string>());
	TEST_ASSERT(str_ti.implements_facet<reflex::facets::range>());

	TEST_ASSERT((str_ti.constructible_from<const char *, std::size_t>()));
	TEST_ASSERT((str_ti.constructible_from<const char *>()));

	TEST_ASSERT((str_ti.constructible_from<std::string_view>()));
	TEST_ASSERT((str_ti.convertible_to<std::string_view>()));

	const auto str0 = str_ti.construct(str_val);
	TEST_ASSERT(!str0.empty() && str0.get<std::string>() == str_val);

	const auto str1 = str_ti.construct(str_val.data());
	TEST_ASSERT(!str1.empty() && str1.get<std::string>() == str_val);
	TEST_ASSERT(str0 == str1);

	const auto str2 = str_ti.construct(str_val.data(), 12);
	TEST_ASSERT(!str2.empty() && str2.get<std::string>() == str_val);
	TEST_ASSERT(str0 == str2);
	TEST_ASSERT(str1 == str2);

	const auto f0 = str0.facet<reflex::facets::string>();
	const auto f1 = str1.facet<reflex::facets::string>();
	const auto f2 = str1.facet<reflex::facets::string>();

	TEST_ASSERT(!f0.empty());
	TEST_ASSERT(!f1.empty());
	TEST_ASSERT(!f2.empty());
	TEST_ASSERT(str_val == f0);
	TEST_ASSERT(str_val == f1);
	TEST_ASSERT(str_val == f2);
}