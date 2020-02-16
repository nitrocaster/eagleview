// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Vector2.hpp"
#include "Angle.hpp"
#include "Math.hpp"

// Column-major order
template <typename Scalar>
struct Matrix23T
{
    Scalar M00, M10;
    Scalar M01, M11;
    Scalar M02, M12;

    Matrix23T() = default;

     constexpr Matrix23T(Scalar m00, Scalar m01, Scalar m02, Scalar m10, Scalar m11, Scalar m12) :
        M00(m00),
        M10(m10),
        M01(m01),
        M11(m11),
        M02(m02),
        M12(m12)
    {}

    constexpr Matrix23T operator*(Matrix23T const &m2) const
    {
        return
        {
            M00*m2.M00 + M01*m2.M10, M00*m2.M01 + M01*m2.M11, M00*m2.M02 + M01*m2.M12 + M02,
            M10*m2.M00 + M11*m2.M10, M10*m2.M01 + M11*m2.M11, M10*m2.M02 + M11*m2.M12 + M12
        };
    }

    constexpr Vector2 operator*(Vector2 v1) const
    {
        return
        {
            M00*v1.X + M01*v1.Y + M02,
            M10*v1.X + M11*v1.Y + M12
        };
    }

    constexpr Matrix23T operator+(Matrix23T const &m2) const
    {
        return
        {
            M00+m2.M00, M01+m2.M01, M02+m2.M02,
            M10+m2.M10, M11+m2.M11, M12+m2.M12
        };
    }

    constexpr Matrix23T operator-(Matrix23T const &m2) const
    {
        return
        {
            M00-m2.M00, M01-m2.M01, M02-m2.M02,
            M10-m2.M10, M11-m2.M11, M12-m2.M12
        };
    }

    constexpr void operator*=(Matrix23T const &m2)
    { *this = *this * m2; }

    constexpr void operator+=(Matrix23T const &m2)
    { *this = *this + m2; }

    constexpr void operator-=(Matrix23T const &m2)
    { *this = *this - m2; }

    bool operator==(Matrix23T const &m2) const
    {
        return std::abs(M00 - m2.M00) <= ScalarEps
            && std::abs(M10 - m2.M10) <= ScalarEps
            && std::abs(M01 - m2.M01) <= ScalarEps
            && std::abs(M11 - m2.M11) <= ScalarEps
            && std::abs(M02 - m2.M02) <= ScalarEps
            && std::abs(M12 - m2.M12) <= ScalarEps;
    }

    bool operator!=(Matrix23T const &m2) const
    { return !(*this == m2); }

    static constexpr Matrix23T Translation(Vector2 offset)
    {
        return
        {
            1, 0, offset.X,
            0, 1, offset.Y
        };
    }

    static Matrix23T Rotation(Angle angle)
    {
        Scalar sin = std::sin(angle.Radians());
        Scalar cos = std::cos(angle.Radians());
        return
        {
            cos, -sin, 0,
            sin, +cos, 0
        };
    }

    static constexpr Matrix23T Scaling(Vector2 scale)
    {
        return
        {
            scale.X, 0, 0,
            0, scale.Y, 0
        };
    }

    static constexpr Matrix23T Inversion(Matrix23T const &m)
    {
        Scalar d = m.M00*m.M11 - m.M01*m.M10;
        return
        {
            +m.M11/d, -m.M01/d, (m.M01*m.M12 - m.M11*m.M02)/d,
            -m.M10/d, +m.M00/d, (m.M10*m.M02 - m.M00*m.M12)/d
        };
    }

    constexpr Vector2 Offset() const
    { return Col2(); }

    constexpr void Offset(Vector2 offset)
    { Col2(offset); }

    constexpr Angle Turn() const
    { return Angle::FromRadians(std::atan2(M10, M00)); }

    void Turn(Angle angle)
    {
        Vector2 scale = Scale();
        M00 = scale.X;
        M11 = scale.Y;
        M01 = 0;
        M10 = 0;
        *this *= Rotation(angle);
    }

    Vector2 Scale() const
    {
        return
        {
            Sign(M00) * Col0().Magnitude(),
            Sign(M11) * Col1().Magnitude()
        };
    }

    void Scale(Vector2 value)
    {
        Vector2 scale = Scale();
        *this *= Scaling(Vector2(1/scale.X, 1/scale.Y));
        *this *= Scaling(value);
    }

    constexpr Vector2 Col0() const
    { return {M00, M10}; }

    constexpr void Col0(Vector2 value)
    {
        M00 = value.X;
        M10 = value.Y;
    }

    constexpr Vector2 Col1() const
    { return {M01, M11}; }

    constexpr void Col1(Vector2 value)
    {
        M01 = value.X;
        M11 = value.Y;
    }

    constexpr Vector2 Col2() const
    { return {M02, M12}; }

    constexpr void Col2(Vector2 value)
    {
        M02 = value.X;
        M12 = value.Y;
    }

private:
    using Limits = std::numeric_limits<Scalar>;

public:
    static constexpr Scalar ScalarEps{Limits::epsilon()};
    static const Matrix23T Identity;
};

using Matrix23f = Matrix23T<float>;
using Matrix23d = Matrix23T<double>;
using Matrix23 = Matrix23f;

template <typename Scalar>
CONSTEXPR_DEF Matrix23T<Scalar> Matrix23T<Scalar>::Identity{1, 0, 0, 0, 1, 0};
