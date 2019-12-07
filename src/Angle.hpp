// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

inline constexpr float Pi = 3.14159265358979323846264338327950288419f;

template <typename T>
struct AngleT;

template <>
struct AngleT<float>
{
    using Scalar = float;
    using Angle = AngleT<Scalar>;

    float Value;

    AngleT() = default;

private:
    constexpr AngleT(Scalar radians) :
        Value(radians)
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

    constexpr float Radians() const
    { return Value; }

    constexpr float Degrees() const
    { return RadiansToDegrees(Value); }

    constexpr Angle &Radians(float radians)
    {
        Value = radians;
        return *this;
    }

    constexpr Angle &Degrees(float degrees)
    {
        Value = DegreesToRadians(degrees);
        return *this;
    }

private:
    static constexpr Scalar ConversionFactor = 360.0f/(2*Pi);
};

using Angle = AngleT<float>;
