/*
 * Created by switchblade on 2023-01-01.
 */

#include <reflex/type_info.hpp>
#include <reflex/object.hpp>

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

class test_object : public reflex::object
{
	REFLEX_DEFINE_OBJECT(test_object)

public:
	constexpr test_object() noexcept = default;
	~test_object() noexcept override = default;
};

int main()
{
	int err = 0;

	auto src = reflex::make_any<int>(1);
	err += !src.type().convertible_to<double>();
	err += !src.type().convertible_to<std::intptr_t>();
	err += !src.type().convertible_to<std::uintptr_t>();

	auto dst = src.try_cast<double>();
	err += dst.empty() || *dst.get<double>() != 1;

	err += (src == dst) != !(src != dst);
	err += !(src >= reflex::any{});
	err += !(src > reflex::any{});
	err += src == reflex::any{};

	const auto obj = test_object{};
	err += !(reflex::type_of(obj) == reflex::type_info::get<test_object>());
	err += !(reflex::type_of(obj).inherits_from<reflex::object>());

	return err;
}