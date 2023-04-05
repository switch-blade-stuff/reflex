/*
 * Created by switchblade on 2023-03-30.
 */

#include "common.hpp"

enum test_enum
{
	test_value_0,
	test_value_1,
};

template<>
struct reflex::type_init<test_enum>
{
	void operator()(reflex::type_factory<test_enum> f) const
	{
		f.enumerate<test_value_0>("test_value_0");
		f.enumerate<test_value_1>("test_value_1");
	}
};

int main()
{
	const auto enum_ti = reflex::type_info::get<test_enum>();

	TEST_ASSERT(enum_ti.is_enum());
	TEST_ASSERT(enum_ti.convertible_to<std::underlying_type_t<test_enum>>());

	TEST_ASSERT(enum_ti.has_enumeration("test_value_0"));
	TEST_ASSERT(enum_ti.has_enumeration("test_value_1"));
	TEST_ASSERT(enum_ti.has_enumeration(reflex::forward_any(test_value_0)));
	TEST_ASSERT(enum_ti.has_enumeration(reflex::forward_any(test_value_1)));

	const auto e0 = enum_ti.enumerate("test_value_0");
	const auto e1 = enum_ti.enumerate("test_value_1");

	TEST_ASSERT(!e0.empty());
	TEST_ASSERT(!e1.empty());
	TEST_ASSERT(e0.is_ref());
	TEST_ASSERT(e1.is_ref());
	TEST_ASSERT(e0.type() == enum_ti);
	TEST_ASSERT(e1.type() == enum_ti);
	TEST_ASSERT(e0.get<test_enum>() == test_value_0);
	TEST_ASSERT(e1.get<test_enum>() == test_value_1);
}