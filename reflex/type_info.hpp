/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "detail/data.hpp"
#include "detail/info.hpp"
#include "detail/database.hpp"
#include "detail/factory.hpp"
#include "detail/object.hpp"
#include "detail/query.hpp"
#include "detail/any.hpp"

#include "detail/facet.hpp"
#include "detail/facets/range.hpp"
#include "detail/facets/tuple.hpp"
#include "detail/facets/string.hpp"
#include "detail/facets/pointer.hpp"
#include "detail/facets/compare.hpp"

#ifdef REFLEX_HEADER_ONLY
#include "detail/spinlock.ipp"
#include "detail/database.ipp"
#include "detail/factory.ipp"
#include "detail/query.ipp"
#include "detail/info.ipp"
#include "detail/any.ipp"
#endif
