// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "ToptestSpace.hpp"

namespace Toptest
{
    class Boardview final
    {
    private:
        std::vector<Vector2> outline;
        ManagedStorage<Part> parts;
        ManagedStorage<Pin> pins;
        ManagedStorage<TestPoint> testPoints;
        std::vector<std::string> netNames;
        ::Origin origin {Vector2::Origin, Angle::FromRadians(0)};

    public:
        ::Origin &Origin()
        { return origin; }
        ::Origin const &Origin() const
        { return origin; }
        std::vector<Vector2> &Outline()
        { return outline; }
        std::vector<Vector2> const &Outline() const
        { return outline; }
        std::vector<std::string> &Nets()
        { return netNames; }
        std::vector<std::string> const &Nets() const
        { return netNames; }
        ManagedStorage<Part> &Parts()
        { return parts; }
        ManagedStorage<Part> const &Parts() const
        { return parts; }
        ManagedStorage<Pin> &Pins()
        { return pins; }
        ManagedStorage<Pin> const &Pins() const
        { return pins; }
        ManagedStorage<TestPoint> &TestPoints()
        { return testPoints; }
        ManagedStorage<TestPoint> const &TestPoints() const
        { return testPoints; }
    };
} // namespace Toptest
