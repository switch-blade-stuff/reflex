/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "detail/type_data.hpp"
#include "detail/type_info.hpp"
#include "detail/database.hpp"
#include "detail/factory.hpp"
#include "detail/object.hpp"
#include "detail/any.hpp"

#include "detail/facet.hpp"
#include "detail/range_facet.hpp"
#include "detail/tuple_facet.hpp"
#include "detail/string_facet.hpp"

#ifdef REFLEX_HEADER_ONLY
#include "detail/spinlock.ipp"
#include "detail/type_info.ipp"
#include "detail/database.ipp"
#include "detail/factory.ipp"
#include "detail/any.ipp"
#endif
