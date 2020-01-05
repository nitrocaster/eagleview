// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include <cstdint>

namespace Tebo
{
    template <size_t DecimalPlaces = 2>
    class Fixed32T
    {
    private:
        static constexpr int32_t Pow(int32_t base, size_t power)
        {
            if (!power)
                return 1;
            return base*Pow(base, power-1);
        }

        static constexpr int32_t decimalMultiplier = Pow(10, DecimalPlaces);
        int32_t value;

    public:
        Fixed32T() { value = 0; }
        Fixed32T(int32_t a) { value = a; }

        operator bool() const
        { return !!value; }
        int32_t ToRawInt() const
        { return value; }
        int32_t Int() const
        { return value / decimalMultiplier; }
        int32_t Frac() const
        { return std::abs(value) % decimalMultiplier; }
        Fixed32T operator+(Fixed32T rhs)
        { return Fixed32T(value + rhs.value); }
        Fixed32T operator-(Fixed32T rhs)
        { return Fixed32T(value - rhs.value); }
        bool operator<(Fixed32T rhs) const
        { return value < rhs.value; }
        bool operator>(Fixed32T rhs) const
        { return value > rhs.value; }
        bool operator<=(Fixed32T rhs) const
        { return value <= rhs.value; }
        bool operator>=(Fixed32T rhs) const
        { return value >= rhs.value; }
        bool operator==(Fixed32T rhs) const
        { return value == rhs.value; }
    };

    using Fixed32 = Fixed32T<2>;

    struct Vector2S
    {
        Fixed32 X, Y;
    };
} // namespace Tebo
