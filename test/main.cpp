/*
 * Created by switchblade on 2023-01-01.
 */

#include <type_name.hpp>
#include <facet.hpp>

#include <vector>
#include <cstdio>

struct test_vtable
{
	int (*get_value)() noexcept;
	void (*set_value)(int i) noexcept;
};

class test_facet : public reflex::facet<test_vtable>
{
	using base_t = reflex::facet<test_vtable>;

public:
	void value(int i) noexcept { base_t::checked_invoke<&test_vtable::set_value, "void value(int) noexcept">(i); }
	int value() const noexcept { return base_t::checked_invoke<&test_vtable::get_value, "int value() const noexcept">(); }
};

static_assert(std::same_as<test_facet::vtable_type, test_vtable>);

int main()
{
	printf("\"%s\"\n", reflex::type_name<int>::value.data());
	printf("\"%s\"\n", reflex::type_name<std::nullptr_t>::value.data());
	printf("\"%s\"\n", reflex::type_name<std::string_view>::value.data());
	printf("\"%s\"\n", reflex::type_name<std::vector<int>>::value.data());
}