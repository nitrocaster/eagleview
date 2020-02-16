// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "BoardFormat.hpp"
#include "ToptestSpace.hpp"
#include "BoardFormatRegistrator.hpp"

namespace CBF
{
    class Board;
}

namespace Toptest
{
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
