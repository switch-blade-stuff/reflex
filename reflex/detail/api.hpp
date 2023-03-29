/*
 * Created by switch_blade on 2023-02-10.
 */

#pragma once

#ifndef REFLEX_HEADER_ONLY
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#define REFLEX_HIDDEN
#define REFLEX_VISIBLE
#if defined(_MSC_VER)
#define REFLEX_API_EXPORT __declspec(dllexport)
#define REFLEX_API_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#define REFLEX_API_EXPORT __attribute__((dllexport))
#define REFLEX_API_IMPORT __attribute__((dllimport))
#endif

#elif __GNUC__ >= 4

#define REFLEX_HIDDEN __attribute__((visibility("hidden")))
#define REFLEX_VISIBLE __attribute__((visibility("default")))
#define REFLEX_API_EXPORT REFLEX_VISIBLE
#define REFLEX_API_IMPORT REFLEX_VISIBLE

#else

#define REFLEX_HIDDEN
#define REFLEX_VISIBLE
#define REFLEX_API_EXPORT
#define REFLEX_API_IMPORT

#endif

#if defined(REFLEX_EXPORT) || defined(REFLEX_LIB_STATIC)
#define REFLEX_PUBLIC REFLEX_API_EXPORT
#else
#define REFLEX_PUBLIC REFLEX_API_IMPORT
#endif

#else
#define REFLEX_VISIBLE
#define REFLEX_HIDDEN
#define REFLEX_PUBLIC
#endif
