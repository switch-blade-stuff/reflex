/*
 * Created by switchblade on 2023-03-30.
 */

#include "common.hpp"

class test_base : public reflex::object
{
	REFLEX_DEFINE_OBJECT(test_base)

public:
	constexpr test_base() noexcept = default;
	~test_base() noexcept override = default;
};
class test_child : public test_base
{
	REFLEX_DEFINE_OBJECT(test_child)

public:
	constexpr test_child() noexcept = default;
	~test_child() noexcept override = default;
};

template<>
struct reflex::type_init<test_child> { void operator()(reflex::type_factory<test_child> f) { f.add_parent<test_base>(); }};

void test_object_info()
{
	TEST_ASSERT(reflex::type_info::get<reflex::object>().is_abstract());

	const auto base_ti = reflex::type_info::get<test_base>();

	TEST_ASSERT(base_ti.inherits_from<reflex::object>());
	TEST_ASSERT(!base_ti.inherits_from<test_child>());
	TEST_ASSERT(!base_ti.is_abstract());

	const auto child_ti = reflex::type_info::get<test_child>();

	TEST_ASSERT(child_ti.inherits_from<reflex::object>());
	TEST_ASSERT(child_ti.inherits_from<test_base>());
	TEST_ASSERT(!child_ti.is_abstract());

	const auto objects = reflex::type_info::query().inherits_from<reflex::object>().types();
	TEST_ASSERT(objects.contains(base_ti));
	TEST_ASSERT(objects.contains(child_ti));
}

void test_object_cast()
{
	const auto base_ti = reflex::type_info::get<test_base>();
	const auto child_ti = reflex::type_info::get<test_child>();

	const auto base = test_base{};
	TEST_ASSERT(reflex::type_of(base) == base_ti);

	auto *base_ptr = &base;
	TEST_ASSERT(reflex::type_of(*base_ptr) == reflex::type_of(base));
	TEST_ASSERT(reflex::type_of(*base_ptr) == base_ti);

	TEST_ASSERT(reflex::object_cast<const reflex::object>(base_ptr) == &base);
	TEST_ASSERT(reflex::object_cast<const test_child>(base_ptr) == nullptr);
	TEST_ASSERT(reflex::object_cast<const test_base>(base_ptr) == &base);

	const auto child = test_child{};
	TEST_ASSERT(reflex::type_of(child) == child_ti);

	base_ptr = &child;
	TEST_ASSERT(reflex::type_of(*base_ptr) == reflex::type_of(child));
	TEST_ASSERT(reflex::type_of(*base_ptr) == child_ti);

	TEST_ASSERT(reflex::object_cast<const reflex::object>(base_ptr) == &child);
	TEST_ASSERT(reflex::object_cast<const test_child>(base_ptr) == &child);
	TEST_ASSERT(reflex::object_cast<const test_base>(base_ptr) == &child);

	const reflex::object *object_ptr = &base;
	TEST_ASSERT(reflex::type_of(*object_ptr) == reflex::type_of(base));
	TEST_ASSERT(reflex::type_of(*object_ptr) == base_ti);

	TEST_ASSERT(reflex::object_cast<const reflex::object>(object_ptr) == &base);
	TEST_ASSERT(reflex::object_cast<const test_child>(object_ptr) == nullptr);
	TEST_ASSERT(reflex::object_cast<const test_base>(object_ptr) == &base);

	object_ptr = &child;
	TEST_ASSERT(reflex::type_of(*object_ptr) == reflex::type_of(child));
	TEST_ASSERT(reflex::type_of(*object_ptr) == child_ti);

	TEST_ASSERT(reflex::object_cast<const reflex::object>(object_ptr) == &child);
	TEST_ASSERT(reflex::object_cast<const test_child>(object_ptr) == &child);
	TEST_ASSERT(reflex::object_cast<const test_base>(object_ptr) == &child);
}

void test_exceptions()
{
	constexpr auto do_test = []<typename U>(U &&value)
	{
		const auto expected_ti = reflex::type_of(value);
		try
		{
			throw std::forward<U>(value);
		}
		catch (const reflex::object &e)
		{
			TEST_ASSERT(reflex::type_of(e) == reflex::type_info::get<U>());
			TEST_ASSERT(reflex::type_of(e) == expected_ti);
		}
	};

	/* Use type_info of void for tests. The value is irrelevant. */
	const auto test_ti = reflex::type_info::get<void>();

	do_test(reflex::facets::bad_facet_function("", ""));
	do_test(reflex::bad_any_cast(test_ti, test_ti));
	do_test(reflex::bad_any_copy(test_ti));

	const auto objects = reflex::type_info::query().inherits_from<reflex::object>().types();
	TEST_ASSERT(objects.contains(reflex::type_info::get<reflex::facets::bad_facet_function>()));
	TEST_ASSERT(objects.contains(reflex::type_info::get<reflex::bad_any_cast>()));
	TEST_ASSERT(objects.contains(reflex::type_info::get<reflex::bad_any_copy>()));
}

int main()
{
	test_object_info();
	test_object_cast();
	test_exceptions();
}