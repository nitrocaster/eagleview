// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include <cmath>

inline constexpr double Pi = 3.14159265358979323846;

template <typename S>
struct AngleT
{
    using Scalar = S;
    using Angle = AngleT<Scalar>;

    Scalar Value;

    AngleT() = default;

private:
    static constexpr Scalar Normalize(Scalar s)
    {
        if (s <= -2*Pi)
        {
            auto k = std::round(-s/(2*Pi));
            s += Scalar(k*2*Pi);
        }
        else if (s >= 2*Pi)
        {
            auto k = std::round(s/(2*Pi));
            s -= Scalar(k*2*Pi);
        }
        return s;
    }

    constexpr AngleT(Scalar radians) :
        Value(Normalize(radians))
    {}

    static constexpr Scalar RadiansToDegrees(Scalar radians)
    { return radians*ConversionFactor; }

    static constexpr Scalar DegreesToRadians(Scalar degrees)
    { return degrees/ConversionFactor; }

public:
    constexpr static Angle FromRadians(Scalar radians)
    { return Angle(radians); }

    constexpr static Angle FromDegrees(Scalar degrees)
    { return Angle(DegreesToRadians(degrees)); }

    constexpr Scalar Radians() const
    { return Value; }

    constexpr Scalar Degrees() const
    { return RadiansToDegrees(Value); }

    constexpr Angle &Radians(Scalar radians)
    {
        Value = radians;
        return *this;
    }

    constexpr Angle &Degrees(Scalar degrees)
    {
        Value = DegreesToRadians(degrees);
        return *this;
    }
    constexpr Angle operator*(Scalar factor) const
    { return Angle(Value*factor); }
    constexpr Angle &operator*=(Scalar factor)
    {
        Value = Normalize(Value*factor);
        return *this;
    }

    constexpr Angle operator/(Scalar factor) const
    { return Angle(Value/factor); }

    constexpr Angle &operator/=(Scalar factor)
    {
        Value = Normalize(Value/factor);
        return *this;
    }

    constexpr Angle operator-() const
    { return Angle(-Value); }

    constexpr Angle operator+() const
    { return *this; }

private:
    static constexpr Scalar ConversionFactor = Scalar(180.0/Pi);
};

using Angle = AngleT<float>;
