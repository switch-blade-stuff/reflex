/*
 * Created by switchblade on 2023-01-01.
 */

#include <reflex/type_info.hpp>

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

static_assert(reflex::type_name_v<int> == "int");
static_assert(reflex::type_name_v<int *> == "int *");
static_assert(reflex::type_name_v<int &> == "int &");
static_assert(reflex::type_name_v<int[]> == "int[]");
static_assert(reflex::type_name_v<int[1]> == "int[1]");
static_assert(reflex::type_name_v<int **> == "int **");
static_assert(reflex::type_name_v<int (&)[]> == "int (&)[]");
static_assert(reflex::type_name_v<int *(&)[]> == "int *(&)[]");
static_assert(reflex::type_name_v<const int> == "const int");
static_assert(reflex::type_name_v<const int *> == "const int *");
static_assert(reflex::type_name_v<int *const> == "int *const");
static_assert(reflex::type_name_v<int **const> == "int **const");
static_assert(reflex::type_name_v<int *const *> == "int *const *");

template<>
struct reflex::type_name<std::string> { static constexpr std::string_view value = "std::string"; };

static_assert(reflex::type_name_v<std::string> == "std::string");
static_assert(reflex::type_name_v<std::wstring> != "std::wstring");

auto test() { return ; }

int main()
{
	auto src = reflex::make_any<int>(1);
	auto dst = src.cast<double>();
	return dst.empty() || *dst.get<double>() != 1;
}