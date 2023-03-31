/*
 * Created by switchblade on 2023-01-01.
 */

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

class test_facet : public reflex::facets::facet<test_vtable>
{
	using base_t = reflex::facets::facet<test_vtable>;

public:
	using base_t::facet;

	void value(int i) noexcept { base_t::checked_invoke<&test_vtable::set_value, "void value(int) noexcept">(instance(), i); }
	int value() const noexcept { return base_t::checked_invoke<&test_vtable::get_value, "int value() const noexcept">(instance()); }
};

static_assert(std::same_as<test_facet::vtable_type, test_vtable>);

template<>
struct reflex::facets::impl_facet<test_facet, int>
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
		TEST_ASSERT(i1 == d0);

		auto i2 = i0.type().construct();
		TEST_ASSERT(reflex::type_of(i2) == reflex::type_info::get<int>());
		TEST_ASSERT(reflex::type_of(i2) == reflex::type_of(int{}));
		TEST_ASSERT(reflex::type_of(i2) == i0.type());
		TEST_ASSERT(!i2.empty() && *i2.get<int>() == 0);
		TEST_ASSERT(i2 != i0);
		TEST_ASSERT(i2 != d0);
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

	{

	}

	return 0;
}