// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include <algorithm> // std::min, std::max

template <typename T>
struct Vector2T;

template <typename T>
struct Box2T;

template <>
struct Box2T<float>
{
    using Scalar = float;
    using Vec2 = Vector2T<Scalar>;
    using Box2 = Box2T<Scalar>;

    Vec2 Min, Max;

    Box2T() = default;

    constexpr Box2T(Vec2 min, Vec2 max) :
        Min(min),
        Max(max)
    {}

    constexpr Box2T(Vec2 center, float radius) :
        Min(center - Vec2(radius, radius)),
        Max(center + Vec2(radius, radius))
    {}

    constexpr Scalar Width() const
    { return Max.X-Min.X; }

    constexpr Scalar Height() const
    { return Max.Y-Min.Y; }

    constexpr Vec2 Center() const
    { return Min + (Max-Min)/2; }

    constexpr Vec2 Size() const
    { return Vec2(Width(), Height()); }

    constexpr bool Contains(Vec2 v) const
    { return Min.X<=v.X && v.X<=Max.X && Min.Y<=v.Y && v.Y<=Max.Y; }

    bool Contains(Scalar x, Scalar y) const
    { return Contains({x, y}); }

    bool Contains(Box2T const &b) const
    { return Contains(b.Min) && Contains(b.Max); }

    Box2T &Merge(Vec2 p)
    {
        Min = {std::min(Min.X, p.X), std::min(Min.Y, p.Y)};
        Max = {std::max(Max.X, p.X), std::max(Max.Y, p.Y)};
        return *this;
    }

    Box2T &Merge(Box2T const &b)
    {
        Merge(b.Min);
        Merge(b.Max);
        return *this;
    }

    Box2T &Shrink(Scalar s)
    {
        Min += s;
        Max -= s;
        return *this;
    }

    Box2T &Grow(Scalar s)
    {
        Min -= s;
        Max += s;
        return *this;
    }
    
    bool operator==(Box2T const &rhs) const
    { return Min==rhs.Min && Max==rhs.Max; }
    
    bool operator!=(Box2T const &rhs) const
    { return Min!=rhs.Min || Max!=rhs.Max; }

    bool IsEmpty() const { return *this == Empty; }

private:
    using Limits = std::numeric_limits<Scalar>;

public:
    static constexpr Scalar ScalarEps {Limits::epsilon()};
    static const Box2 Empty;
};

using Box2 = Box2T<float>;

inline constexpr Box2 Box2::Empty {Vector2::MinValue, Vector2::MinValue};
