// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include <limits> // std::numeric_limits
#include <cmath> // std::fabs

template <typename T>
struct Vector2T;

template <>
struct Vector2T<float>
{
    using Scalar = float;
    using Vec2 = Vector2T<Scalar>;

    union
    {
        struct
        {
            Scalar X, Y;
        };
        Scalar S[2];
    };

    Vector2T() = default;

    constexpr Vector2T(Scalar x, Scalar y) :
        X(x),
        Y(y)
    {}

    constexpr Scalar &operator[](size_t i)
    { return S[i]; }

    constexpr Scalar const &operator[](size_t i) const
    { return S[i]; }

    constexpr Vec2 operator+(Vec2 v) const
    { return Vec2(X+v.X, Y+v.Y); }

    constexpr Vec2 operator-(Vec2 v) const
    { return Vec2(X-v.X, Y-v.Y); }

    constexpr Vec2 operator+(Scalar s) const
    { return Vec2(X+s, Y+s); }

    constexpr Vec2 operator-(Scalar s) const
    { return Vec2(X-s, Y-s); }

    constexpr Vec2 operator*(Scalar s) const
    { return Vec2(X*s, Y*s); }

    constexpr Vec2 operator/(Scalar s) const
    { return Vec2(X/s, Y/s); }

    constexpr Vec2 &operator+=(Scalar s)
    { return *this = *this + s; }

    constexpr Vec2 &operator-=(Scalar s)
    { return *this = *this - s; }

    constexpr Vec2 &operator+=(Vec2 v)
    { return *this = *this + v; }

    constexpr Vec2 &operator-=(Vec2 v)
    { return *this = *this - v; }

    constexpr Vec2 operator-() const
    { return Vec2(-X, -Y); }

    constexpr Vec2 operator+() const
    { return *this; }

    constexpr Scalar SqrLength() const
    { return X*X + Y*Y; }

    constexpr bool operator<(Vec2 v2) const
    { return SqrLength() < v2.SqrLength(); }

    constexpr bool operator>(Vec2 v2) const
    { return SqrLength() > v2.SqrLength(); }

    constexpr bool operator<=(Vec2 v2) const
    { return SqrLength() <= v2.SqrLength(); }

    constexpr bool operator>=(Vec2 v2) const
    { return SqrLength() >= v2.SqrLength(); }

    bool operator==(Vec2 v2) const
    { return std::fabs(X-v2.X) <= ScalarEps && std::fabs(Y-v2.Y) <= ScalarEps; }

    bool operator!=(Vec2 v2) const
    { return !(*this==v2); }

private:
    using Limits = std::numeric_limits<Scalar>;

public:
    static constexpr Scalar ScalarEps {Limits::epsilon()};
    static const Vec2 MinValue;
    static const Vec2 MaxValue;
    static const Vec2 Epsilon;
    static const Vec2 Origin;
};

using Vector2 = Vector2T<float>;

inline constexpr Vector2 Vector2::MinValue {Limits::min(), Limits::min()};
inline constexpr Vector2 Vector2::MaxValue {Limits::max(), Limits::max()};
inline constexpr Vector2 Vector2::Epsilon {ScalarEps, ScalarEps};
inline constexpr Vector2 Vector2::Origin {0, 0};
