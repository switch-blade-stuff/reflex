/*
 * Created by switchblade on 2023-03-31.
 */

#include "common.hpp"

int main()
{
	const auto ptr_ti = reflex::type_info::get<int *>();
	TEST_ASSERT(ptr_ti.implements_facet<reflex::facets::pointer>());

	auto i = 1;
	const auto ptr = reflex::make_any<int *>(&i);
	TEST_ASSERT((!ptr.empty() && ptr.get<int *>() == &i));

	const auto pf = ptr.facet<reflex::facets::pointer>();
	TEST_ASSERT(!pf.empty());
	TEST_ASSERT(pf.data() == &i);

	auto pr = pf.deref();
	TEST_ASSERT(!pr.empty());
	TEST_ASSERT(pr.is_ref());
	TEST_ASSERT(&pr.as<int>() == &i);
}