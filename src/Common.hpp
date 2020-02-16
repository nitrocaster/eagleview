// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include <cstdint>
#include <cstdio>

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#define FUNCTION_NAME __FUNCTION__
// workaround for MSVC bug
#define CONSTEXPR_DEF const
#elif defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#define FUNCTION_NAME __PRETTY_FUNCTION__
#define CONSTEXPR_DEF constexpr
#else
#error "Unsupported compiler"
#endif

inline void Fail(char const *expr, char const *func,
    char const *file, int line, char const *desc)
{
    printf("\n! FATAL ERROR\n"
        "  Expression: %s\n"
        "  Function: %s\n"
        "  File: %s:%d\n"
        "  Description: %s\n",
        expr, func, file, line, desc);
    DEBUG_BREAK();
}

#define R_ASSERT(expr) \
    do \
    { \
        if (!(expr)) \
            Fail(#expr, FUNCTION_NAME, __FILE__, __LINE__, "assertion failed"); \
    } while (0)
