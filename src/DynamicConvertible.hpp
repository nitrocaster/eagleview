// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

template <class ...Tx>
class DynamicConvertible;

template <class T, class ...Tx>
class DynamicConvertible<T, Tx...> : public DynamicConvertible<Tx...>
{
public:
    virtual ~DynamicConvertible() = default;
    operator T *() { return dynamic_cast<T *>(this); }
    operator T const *() const { return dynamic_cast<T const *>(this); }
};

template <>
class DynamicConvertible<>
{};
