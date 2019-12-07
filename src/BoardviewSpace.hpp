// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Vector2.hpp"
#include "Angle.hpp"

using NetID = size_t;

enum class BoardLayer
{
    Top = 1,
    Bottom = 2,
    Multilayer = Top|Bottom
};

struct Origin
{
    Vector2 Pos;
    Angle Turn;
};
