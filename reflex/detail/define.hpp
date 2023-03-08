/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <version>
#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#define REFLEX_PRETTY_FUNC __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define REFLEX_PRETTY_FUNC __PRETTY_FUNCTION__
#endif

#ifdef REFLEX_HAS_SEKHMET
#define REFLEX_NAMESPACE sek
#define SEK_PRETTY_FUNC REFLEX_PRETTY_FUNC
#else
#define REFLEX_NAMESPACE reflex
#endif