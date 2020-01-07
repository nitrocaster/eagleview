#pragma once

#define R_ASSERT(expr) do { if (!(expr)) __debugbreak(); } while (0)
