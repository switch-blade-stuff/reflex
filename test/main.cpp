/*
 * Created by switchblade on 2023-01-01.
 */

#include <reflex/type_info.hpp>
#include <reflex/object.hpp>
#include <reflex/facet.hpp>

#ifndef NDEBUG
#include <cassert>
#define TEST_ASSERT(cnd) assert((cnd))
#else
#define TEST_ASSERT(cnd) do { if (!(cnd)) std::terminate(); } while (false)
#endif

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

struct test_vtable
{
	void (*set_value)(reflex::any &, int);
	int (*get_value)(const reflex::any &);
};

class test_facet : public reflex::facet<test_vtable>
{
	using base_t = reflex::facet<test_vtable>;

public:
	using base_t::facet;

	void value(int i) noexcept { base_t::checked_invoke<&test_vtable::set_value, "void value(int) noexcept">(instance(), i); }
	int value() const noexcept { return base_t::checked_invoke<&test_vtable::get_value, "int value() const noexcept">(instance()); }
};

static_assert(std::same_as<test_facet::vtable_type, test_vtable>);

template<>
struct reflex::impl_facet<test_facet, int>
{
	constexpr static auto value = test_vtable{
			.set_value = +[](any &obj, int i) { *obj.get<int>() = i; },
			.get_value = +[](const any &obj) { return *obj.get<int>(); },
	};
};

int main()
{
	{
		auto i0 = reflex::make_any<int>(1);
		TEST_ASSERT(i0.type().convertible_to<double>());
		TEST_ASSERT(i0.type().convertible_to<std::intmax_t>());
		TEST_ASSERT(i0.type().convertible_to<std::uintmax_t>());

		TEST_ASSERT(reflex::type_of(i0) == reflex::type_info::get<int>());
		TEST_ASSERT(reflex::type_of(i0) == reflex::type_of(int{}));
		TEST_ASSERT(reflex::type_of(i0) == i0.type());

		auto d0 = i0.try_cast<double>();
		TEST_ASSERT(!d0.empty() && *d0.get<double>() == 1);

		TEST_ASSERT(reflex::type_of(d0) == reflex::type_info::get<double>());
		TEST_ASSERT(reflex::type_of(d0) == reflex::type_of(double{}));
		TEST_ASSERT(reflex::type_of(d0) == d0.type());;

		TEST_ASSERT(i0.cast<std::intmax_t>() == d0.cast<std::intmax_t>());
		TEST_ASSERT(i0 != reflex::any{});
		TEST_ASSERT(i0 >= reflex::any{});
		TEST_ASSERT(i0 > reflex::any{});

		TEST_ASSERT(reflex::type_info::get<reflex::object>().is_abstract());

		auto i1 = i0.type().construct(i0);
		TEST_ASSERT(reflex::type_of(i1) == reflex::type_info::get<int>());
		TEST_ASSERT(reflex::type_of(i1) == reflex::type_of(int{}));
		TEST_ASSERT(reflex::type_of(i1) == i0.type());
		TEST_ASSERT(!i1.empty() && *i1.get<int>() == 1);
		TEST_ASSERT(i1 == i0);

		auto i2 = i0.type().construct();
		TEST_ASSERT(reflex::type_of(i2) == reflex::type_info::get<int>());
		TEST_ASSERT(reflex::type_of(i2) == reflex::type_of(int{}));
		TEST_ASSERT(reflex::type_of(i2) == i0.type());
		TEST_ASSERT(!i2.empty() && *i2.get<int>() == 0);
		TEST_ASSERT(i2 != i0);
	}

	{
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

		const auto base = test_base{};
		TEST_ASSERT(reflex::type_of(base) == reflex::type_info::get<test_base>());
		TEST_ASSERT(reflex::type_of(base).inherits_from<reflex::object>());

		reflex::type_info::reflect<test_child>().base<test_base>();

		auto *base_ptr = &base;
		TEST_ASSERT(reflex::type_of(*base_ptr) == reflex::type_info::get<test_base>());
		TEST_ASSERT(reflex::type_of(base) == reflex::type_of(*base_ptr));
		TEST_ASSERT(reflex::object_cast<const reflex::object>(base_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_child>(base_ptr) == nullptr);
		TEST_ASSERT(reflex::object_cast<const test_base>(base_ptr) != nullptr);

		const auto child = test_child{};
		TEST_ASSERT(reflex::type_of(child) == reflex::type_info::get<test_child>());
		TEST_ASSERT(reflex::type_of(child).inherits_from<reflex::object>());
		TEST_ASSERT(reflex::type_of(child).inherits_from<test_base>());

		base_ptr = &child;
		TEST_ASSERT(reflex::type_of(*base_ptr) == reflex::type_info::get<test_child>());
		TEST_ASSERT(reflex::type_of(child) == reflex::type_of(*base_ptr));
		TEST_ASSERT(reflex::object_cast<const reflex::object>(base_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_child>(base_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_base>(base_ptr) != nullptr);

		const reflex::object *object_ptr = &base;
		TEST_ASSERT(reflex::type_of(*object_ptr) == reflex::type_info::get<test_base>());
		TEST_ASSERT(reflex::type_of(base) == reflex::type_of(*object_ptr));
		TEST_ASSERT(reflex::object_cast<const reflex::object>(object_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_child>(object_ptr) == nullptr);
		TEST_ASSERT(reflex::object_cast<const test_base>(object_ptr) != nullptr);

		object_ptr = &child;
		TEST_ASSERT(reflex::type_of(*object_ptr) == reflex::type_info::get<test_child>());
		TEST_ASSERT(reflex::type_of(child) == reflex::type_of(*object_ptr));
		TEST_ASSERT(reflex::object_cast<const reflex::object>(object_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_child>(object_ptr) != nullptr);
		TEST_ASSERT(reflex::object_cast<const test_base>(object_ptr) != nullptr);
	}

	{
		const auto str_ti = reflex::type_info::reflect<std::string>()
				.ctor<const char *, std::size_t>()
				.ctor<const char *>()
				.type();

		TEST_ASSERT((str_ti.constructible_from<const char *, std::size_t>()));
		TEST_ASSERT((str_ti.constructible_from<const char *>()));

		const auto str0 = str_ti.construct("hello, world");
		TEST_ASSERT(!str0.empty() && *str0.get<std::string>() == "hello, world");

		const auto str1 = str_ti.construct("hello, world", 12);
		TEST_ASSERT(!str1.empty() && *str1.get<std::string>() == "hello, world");
		TEST_ASSERT(str0 == str1);

		reflex::type_info::reset<std::string>();

		TEST_ASSERT((!str_ti.constructible_from<const char *, std::size_t>()));
		TEST_ASSERT((!str_ti.constructible_from<const char *>()));
	}

	{
		const auto int_ti = reflex::type_info::reflect<int>().facet<test_facet>().type();
		TEST_ASSERT(int_ti.implements_facet<test_facet>());

		auto i = reflex::make_any<int>();
		auto fi = i.facet<test_facet>();

		TEST_ASSERT(fi.instance().data() == i.data());
		TEST_ASSERT(fi.instance().is_ref());

		fi.value(0);
		TEST_ASSERT(fi.value() == 0);
		fi.value(1);
		TEST_ASSERT(fi.value() == 1);

		reflex::type_info::reset<int>();
		TEST_ASSERT(!int_ti.implements_facet<test_facet>());
	}
}