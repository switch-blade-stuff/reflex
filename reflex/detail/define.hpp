/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <version>
#include <cstddef>
#include <cstdint>

#include "api.hpp"

#if defined(_MSC_VER)
#define REFLEX_PRETTY_FUNC __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define REFLEX_PRETTY_FUNC __PRETTY_FUNCTION__
#endif