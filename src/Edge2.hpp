// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "Vector2.hpp"

template <typename Scalar>
struct Edge2T
{
    using Vec2 = Vector2T<Scalar>;
    
    Vec2 A, B;

    Edge2T() = default;

    constexpr Edge2T(Vec2 a, Vec2 b) :
        A(a),
        B(b)
    {}

    constexpr Vec2 &operator[](size_t i)
    {
        switch (i)
        {
        case 0: return A;
        case 1: return B;
        default: R_ASSERT(!"Index out of range");
        }
    }

    constexpr Vec2 const &operator[](size_t i) const
    { return const_cast<Edge2T &>(*this)[i]; }

    constexpr Scalar SqrLength() const
    { return (B-A).SqrLength(); }

    Scalar Length() const
    { return (B-A).Length(); }

    template <typename T>
    operator Edge2T<T>() const
    { return {A, B}; }
};

using Edge2 = Edge2T<float>;
using Edge2f = Edge2T<float>;
using Edge2d = Edge2T<double>;
using Edge2i = Edge2T<int32_t>;
