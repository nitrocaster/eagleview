// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "ToptestSpace.hpp"
#include "ToptestBoardview.hpp"
#include "TeboBoard.hpp"

namespace Toptest
{
    class TeboImporter final
    {
    private:
        Boardview &brd;

        uint32_t FindLayerObject(Tebo::Board const &src, Tebo::LayerType type)
        {
            for (uint32_t i = 0; i < src.Layers.size(); i++)
            {
                if (src.Layers[i]->Type == type)
                    return i;
            }
            return -1;
        }
        
        Tebo::ThroughLayer const *GetThroughLayer(Tebo::Board const &src, uint32_t index)
        {
            if (index >= src.Layers.size())
                return nullptr;
            return dynamic_cast<Tebo::ThroughLayer const *>(src.Layers[index].get());
        }

        Tebo::LogicLayer const *GetLogicLayer(Tebo::Board const &src, uint32_t index)
        {
            if (index >= src.Layers.size())
                return nullptr;
            return dynamic_cast<Tebo::LogicLayer const *>(src.Layers[index].get());
        }

        void BuildOutline(Tebo::Board const &src)
        {
            auto index = FindLayerObject(src, Tebo::LayerType::Roul);
            auto profile = GetThroughLayer(src, index);
            if (!profile)
                return;
            for (auto const &slot : profile->DrillSlots)
            {
                brd.Outline().push_back(slot.Begin);
                brd.Outline().push_back(slot.End);
            }
        }

        BoardLayer GetLayerCode(Tebo::Board const &src, uint32_t index)
        {
            auto layer = GetLogicLayer(src, index);
            R_ASSERT(layer != nullptr);
            switch (layer->Type)
            {
            case Tebo::LayerType::Top: return BoardLayer::Top;
            case Tebo::LayerType::Bottom: return BoardLayer::Bottom;
            default:
            {
                R_ASSERT("Invalid layer type");
                return BoardLayer::Top;
            }
            }
        }

        void ProcessLogicLayers(Tebo::Board const &src)
        {
            // XXX: support single-sided boards?
            auto topIndex = FindLayerObject(src, Tebo::LayerType::Top);
            auto top = GetLogicLayer(src, topIndex);
            if (!top)
                return;
            auto bottomIndex = FindLayerObject(src, Tebo::LayerType::Bottom);
            auto bottom = GetLogicLayer(src, bottomIndex);
            if (!bottom)
                return;
            brd.Parts().reserve(src.Parts.size());
            brd.Pins().reserve(top->Pads.size() + bottom->Pads.size());
            for (Tebo::Part const &part : src.Parts)
            {
                if (part.Layer != topIndex && part.Layer != bottomIndex)
                    continue;
                auto dstPart = std::make_unique<Part>();
                dstPart->Name(part.Name);
                dstPart->Layer(GetLayerCode(src, part.Layer));
                dstPart->FirstPin(brd.Pins().size());
                dstPart->PinCount(part.Pins.size());
                brd.Parts().push_back(std::move(dstPart));
                // note 1: assuming pins are sorted by id in ascending order
                // note 2: in Tebo board parts can not have pins on multiple layers
                auto srcLayer = part.Layer == topIndex ? top : bottom;
                for (Tebo::Pin const &pin : part.Pins)
                {
                    auto padIndex = pin.Handle/8;
                    auto dstPin = std::make_unique<Pin>();
                    dstPin->Name(pin.Name);
                    dstPin->Layer(GetLayerCode(src, part.Layer));
                    R_ASSERT(padIndex < srcLayer->Pads.size());
                    auto const &pad = srcLayer->Pads[padIndex];
                    auto pos = pad.Pos;
                    dstPin->Location(pos);
                    dstPin->Net(pad.Net+1);
                    brd.Pins().push_back(std::move(dstPin));
                }
            }
        }

    public:
        TeboImporter(Boardview &brd) : brd(brd)
        {}

        void Import(Tebo::Board const &src)
        {
            BuildOutline(src);
            brd.Nets().reserve(src.Nets.size());
            for (auto const &net : src.Nets)
                brd.Nets().push_back(net);
            // XXX: don't include testpoints here
            ProcessLogicLayers(src);
        }
    };
} // namespace Toptest
