// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "BoardFormat.hpp"
#include "BoardFormatRegistrator.hpp"
#include "Vector2.hpp"
#include "Box2.hpp"
#include <string>
#include <vector>
#include <memory>

namespace CBF
{
    class Board;
}

namespace Toptest
{
    using NetID = size_t;

    enum class BoardLayer
    {
        Multilayer = 0,
        Top = 1,
        Bottom = 2
    };

    class Contact
    {
    private:
        std::string name;
        BoardLayer layer;
        Vector2i location;
        NetID net;
    public:
        virtual ~Contact() = 0;
        Vector2i Location() const { return location; }
        void Location(Vector2i v) { location = v; }
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
        Box2i bbox = Box2i::Empty;
    public:
        std::string const &Name() const { return name; }
        void Name(std::string n) { name = std::move(n); }
        BoardLayer Layer() const { return layer; }
        void Layer(BoardLayer l) { layer = l; }
        size_t FirstPin() const { return firstPin; }
        void FirstPin(size_t fp) { firstPin = fp; }
        size_t PinCount() const { return pinCount; }
        void PinCount(size_t pc) { pinCount = pc; }
        Box2i BBox() const { return bbox; }
        void BBox(Box2i const &b) { bbox = b; }
    };

    class Pin final : public Contact
    {};

    class TestPoint final : public Contact
    {};

    template <typename T>
    using ManagedStorage = std::vector<std::unique_ptr<T>>;

    class Board : public BoardFormat
    {
    private:
        std::vector<Vector2i> outline;
        ManagedStorage<Part> parts;
        ManagedStorage<Pin> pins;
        ManagedStorage<TestPoint> testPoints;
        std::vector<std::string> netNames;

    public:
        std::vector<Vector2i> &Outline()
        { return outline; }
        std::vector<Vector2i> const &Outline() const
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

        class Rep : public BoardFormatRep
        {
        public:
            Rep() : BoardFormatRep((Board *)0)
            {}
            virtual char const *Tag() const override { return "toptest"; }
            virtual char const *Desc() const override { return "Toptest board view (*.BRD)"; }
            virtual bool CanExport() const override { return true; }            
        };

        virtual void Export(CBF::Board const &cbf, std::ostream &fs) override;
        virtual BoardFormatRep const &Frep() const override;

    private:
        void Import(CBF::Board const &cbf);
        void Save(std::ostream &fs);

        void BuildOutline(CBF::Board const &src);
        void ProcessLogicLayers(CBF::Board const &src);
    };
} // namespace Toptest
