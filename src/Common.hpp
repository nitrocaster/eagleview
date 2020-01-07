#pragma once

#include <cstdio>

inline void Fail(char const *expr, char const *func,
    char const *file, int line, char const *desc)
{
    printf("\n! FATAL ERROR\n"
        "  Expression: %s\n"
        "  Function: %s\n"
        "  File: %s:%d\n"
        "  Description: %s\n",
        expr, func, file, line, desc);
    __debugbreak();
}

#define R_ASSERT(expr) \
    do \
    { \
        if (!(expr)) \
            Fail(#expr, __FUNCTION__, __FILE__, __LINE__, "assertion failed"); \
    } while (0)
