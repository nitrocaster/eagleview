// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

template <typename T>
constexpr int Sign(T value)
{ return (T(0) < value) - (value < T(0)); }
