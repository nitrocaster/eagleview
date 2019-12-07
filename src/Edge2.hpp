// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Vector2.hpp"

template <typename T>
struct Edge2T;

template <>
struct Edge2T<float>
{
    using Scalar = float;
    using Vec2 = Vector2T<Scalar>;
    
    union
    {
        struct
        {
            Vec2 A, B;
        };
        Vec2 V[2];
    };

    Edge2T() = default;

    constexpr Edge2T(Vec2 a, Vec2 b) :
        A(a),
        B(b)
    {}

    constexpr Vec2 &operator[](size_t i)
    { return V[i]; }

    constexpr Vec2 const &operator[](size_t i) const
    { return V[i]; }

    constexpr Scalar SqrLength() const
    { return (B-A).SqrLength(); }
};

using Edge2 = Edge2T<float>;
