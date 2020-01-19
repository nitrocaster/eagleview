// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "BoardviewSpace.hpp"
#include "Vector2.hpp"
#include "Box2.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Toptest
{
    class Contact
    {
    private:
        std::string name;
        BoardLayer layer;
        Vector2 location;
        NetID net;
    public:
        virtual ~Contact() = 0;
        Vector2 Location() const { return location; }
        void Location(Vector2 v) { location = v; }
        NetID Net() const { return net; }
        void Net(NetID id) { net = id; }
        std::string const &Name() const { return name; }
        void Name(std::string n) { name = std::move(n); }
        BoardLayer Layer() const { return layer; }
        void Layer(BoardLayer l) { layer = l; }
    };

    inline Contact::~Contact() = default;

    class Part final
    {
    private:
        std::string name;
        BoardLayer layer;
        size_t firstPin, pinCount;
    public:
        std::string const &Name() const { return name; }
        void Name(std::string n) { name = std::move(n); }
        BoardLayer Layer() const { return layer; }
        void Layer(BoardLayer l) { layer = l; }
        size_t FirstPin() const { return firstPin; }
        void FirstPin(size_t fp) { firstPin = fp; }
        size_t PinCount() const { return pinCount; }
        void PinCount(size_t pc) { pinCount = pc; }
        // XXX: calculate part bbox
        Box2 BBox() const { return Box2::Empty; }
    };

    class Pin final : public Contact
    {};

    class TestPoint final : public Contact
    {};

    template <typename T>
    using ManagedStorage = std::vector<std::unique_ptr<T>>;
} // namespace Toptest
