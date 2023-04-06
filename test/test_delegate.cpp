/*
 * Created by switchblade on 2023-04-06.
 */

#include <reflex/delegate.hpp>

#include "common.hpp"

static_assert(std::same_as<decltype(reflex::delegate{[]() { return 0; }}), reflex::delegate<int()>>);
static_assert(std::same_as<decltype(reflex::delegate{[](int i) { return i; }}), reflex::delegate<int(int)>>);

int main()
{
	auto f0 = reflex::delegate{[]() { return 0; }};
	auto f1 = reflex::delegate{[]() { return 1; }};
	TEST_ASSERT(f0);
	TEST_ASSERT(f1);
	TEST_ASSERT(f0() == 0);
	TEST_ASSERT(f1() == 1);

	TEST_ASSERT(f0 = f1);
	TEST_ASSERT(f0() == 1);

	constexpr auto test_func = +[](void *data) { return *static_cast<const int *>(data); };
	int test_val = 0;

	TEST_ASSERT(f0 = reflex::delegate(test_func, &test_val));
	TEST_ASSERT(f1 = reflex::delegate(test_func, &test_val));
	TEST_ASSERT(f0() == test_val);
	TEST_ASSERT(f1() == test_val);
	test_val = 1;
	TEST_ASSERT(f0() == test_val);
	TEST_ASSERT(f1() == test_val);

	TEST_ASSERT(f0 = [i = 0](){ return i; });
	TEST_ASSERT(f1 = [i = 1](){ return i; });
	TEST_ASSERT(f0() == 0);
	TEST_ASSERT(f1() == 1);

	struct test_struct { int value; };
	auto s = std::make_shared<test_struct>();

	TEST_ASSERT(f0 = reflex::member_delegate<&test_struct::value>(s.get()));
	TEST_ASSERT(f0() == s->value);
	TEST_ASSERT(f0 = reflex::member_delegate<&test_struct::value>(s));
	TEST_ASSERT(f0() == s->value);
}