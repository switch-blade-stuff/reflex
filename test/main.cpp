/*
 * Created by switchblade on 2023-01-01.
 */

#include <reflex/type_info.hpp>
#include <reflex/object.hpp>
#include <reflex/facet.hpp>

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

int main()
{
	int err = 0;

	auto i0 = reflex::make_any<int>(1);
	err += !i0.type().convertible_to<double>();
	err += !i0.type().convertible_to<std::intmax_t>();
	err += !i0.type().convertible_to<std::uintmax_t>();

	err += !(reflex::type_of(i0) == reflex::type_info::get<int>());
	err += !(reflex::type_of(i0) == reflex::type_of(int{}));
	err += !(reflex::type_of(i0) == i0.type());

	auto d0 = i0.try_cast<double>();
	err += d0.empty() || *d0.get<double>() != 1;

	err += !(reflex::type_of(d0) == reflex::type_info::get<double>());
	err += !(reflex::type_of(d0) == reflex::type_of(double{}));
	err += !(reflex::type_of(d0) == d0.type());;

	err += i0.cast<std::intmax_t>() != d0.cast<std::intmax_t>();
	err += !(i0 >= reflex::any{});
	err += !(i0 > reflex::any{});
	err += i0 == reflex::any{};

	err += !reflex::type_info::get<reflex::object>().is_abstract();

	auto i1 = i0.type().construct(i0);
	err += !(reflex::type_of(i1) == reflex::type_info::get<int>());
	err += !(reflex::type_of(i1) == reflex::type_of(int{}));
	err += !(reflex::type_of(i1) == i0.type());
	err += i1.empty() || *i1.get<int>() != 1;
	err += i1 != i0;

	auto i2 = i0.type().construct();
	err += !(reflex::type_of(i2) == reflex::type_info::get<int>());
	err += !(reflex::type_of(i2) == reflex::type_of(int{}));
	err += !(reflex::type_of(i2) == i0.type());
	err += i2.empty() || *i2.get<int>() != 0;
	err += i2 == i0;

	const auto base = test_base{};
	err += !(reflex::type_of(base) == reflex::type_info::get<test_base>());
	err += !(reflex::type_of(base).inherits_from<reflex::object>());

	auto *base_ptr = &base;
	err += !(reflex::type_of(*base_ptr) == reflex::type_info::get<test_base>());
	err += !(reflex::type_of(base) == reflex::type_of(*base_ptr));
	err += reflex::object_cast<const reflex::object>(base_ptr) == nullptr;
	err += reflex::object_cast<const test_base>(base_ptr) == nullptr;

	const auto child = test_child{};
	err += !(reflex::type_of(child) == reflex::type_info::get<test_child>());
	err += !(reflex::type_of(child).inherits_from<reflex::object>());

	base_ptr = &child;
	err += !(reflex::type_of(*base_ptr) == reflex::type_info::get<test_child>());
	err += !(reflex::type_of(child) == reflex::type_of(*base_ptr));
	err += reflex::object_cast<const reflex::object>(base_ptr) == nullptr;
	err += reflex::object_cast<const test_child>(base_ptr) == nullptr;
	err += reflex::object_cast<const test_base>(base_ptr) == nullptr;

	return err;
}