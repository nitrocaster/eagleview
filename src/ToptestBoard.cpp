// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#include "ToptestBoard.hpp"
#include "CBF/Board.hpp"
#include "OutlineBuilder.hpp"
#include "Matrix23.hpp"
#include <optional>

namespace Toptest
{
    static BoardLayer DecodeLayer(int code)
    {
        switch (code)
        {
        case 0:
            return BoardLayer::Multilayer;
        case 1:
        case 5:
            return BoardLayer::Top;
        case 2:
        case 10:
            return BoardLayer::Bottom;
        default:
            return BoardLayer::Top;
        }
    }

    static int EncodeLayer(BoardLayer layer)
    {
        switch (layer)
        {
        default:
        case BoardLayer::Top:
            return 1;
        case BoardLayer::Bottom:
            return 2;
        case BoardLayer::Multilayer:
            return 0;
        }
    }

    class StreamWriter
    {
    protected:
        std::ostream &os;
        bool flipY = false;
        bool shift = false;
        int32_t outlineHeight = 0;

        void SetTransform(BoardLayer layer)
        {
            switch (layer)
            {
            case BoardLayer::Top:
                flipY = true;
                shift = true;
                break;
            }
        }

        void ResetTransform()
        {
            flipY = false;
            shift = false;
        }

    public:
        StreamWriter(std::ostream &s) : os(s)
        {}

        void OutlineHeight(int32_t h) { outlineHeight = h; }

        template <typename T, bool Condition = false>
        static void Fail() { static_assert(Condition); }

        template <typename T>
        void Write(T) { Fail<T>(); }

        template <typename T>
        void Write(T const *x) { Fail<T>(); }

        template <typename T>
        void Write(T *x)
        { Write(const_cast<T const *>(x)); }

        void Write(char const *s)
        { os.write(s, std::strlen(s)); }

        void Write(char c)
        { os.put(c); }

        void Write(std::string const &s)
        { os.write(s.data(), s.size()); }

        void Write(std::string &&s)
        { os.write(s.data(), s.size()); }

        void Write(int32_t v)
        {
            char buf[16];
            _itoa_s(v, buf, 10);
            Write(buf);
        }

        void Write(int64_t v)
        {
            char buf[24];
            _i64toa_s(v, buf, sizeof(buf), 10);
            Write(buf);
        }

        void Write(size_t v)
        {
            char buf[24];
            _ui64toa_s(v, buf, sizeof(buf), 10);
            Write(buf);
        }

        void Write(BoardLayer layer)
        { Write(EncodeLayer(layer)); }

        template <typename... Args>
        void Write(Args const &...args)
        { (Write(args), ...); }

        void Write(Vector2i v)
        {
            if (shift)
                v.Y -= outlineHeight;
            Write(v.X, ' ', flipY ? -v.Y : v.Y);
        }

        void Write(Box2i b)
        { Write(b.Min, ' ', b.Max); }

        void Write(Part const *p)
        {
            Write(p->Name(), ' ', p->BBox(), ' ', p->FirstPin(), ' ', p->Layer());
        }

        void Write(Pin const *p)
        {
            SetTransform(p->Layer());
            Write(p->Location(), ' ', p->Net(), ' ', p->Layer());
            ResetTransform();
        }

        void Write(TestPoint const *n)
        {
            SetTransform(n->Layer());
            Write(n->Location(), ' ', n->Net(), ' ', n->Layer());
            ResetTransform();
        }
    };
    
    static uint32_t FindLayerObject(CBF::Board const &src, CBF::LayerType role)
    {
        for (uint32_t i = 0; i < src.Layers.size(); i++)
        {
            if (src.Layers[i]->Type == role)
                return i;
        }
        return -1;
    }

    static CBF::DrillLayer const *GetThroughLayer(CBF::Board const &src, uint32_t index)
    {
        if (index >= src.Layers.size())
            return nullptr;
        return *src.Layers[index].get();
    }

    static CBF::LogicLayer const *GetLogicLayer(CBF::Board const &src, uint32_t index)
    {
        if (index >= src.Layers.size())
            return nullptr;
        return *src.Layers[index].get();
    }

    void Board::BuildOutline(CBF::Board const &src)
    {
        auto index = FindLayerObject(src, CBF::LayerType::Route);
        auto profile = GetThroughLayer(src, index);
        if (!profile)
            return;
        OutlineBuilder outlineBuilder;
        for (auto const &slot : profile->Slots)
            outlineBuilder.AddEdge(slot);
        outlineBuilder.Build(outline);      
    }

    static BoardLayer GetLayerCode(CBF::Board const &src, uint32_t index)
    {
        CBF::LogicLayer const *layer = *src.Layers[index];
        R_ASSERT(layer != nullptr);
        switch (layer->Type)
        {
        case CBF::LayerType::Multilayer: return BoardLayer::Multilayer;
        case CBF::LayerType::Top: return BoardLayer::Top;
        case CBF::LayerType::Bottom: return BoardLayer::Bottom;
        default:
            R_ASSERT("Invalid layer type");
            return BoardLayer::Top;
        }
    }

    void Board::ProcessLogicLayers(CBF::Board const &src)
    {
        // XXX: support single-sided boards?
        auto multiIndex = FindLayerObject(src, CBF::LayerType::Multilayer);
        auto multi = GetLogicLayer(src, multiIndex);
        if (!multi)
            return;
        auto topIndex = FindLayerObject(src, CBF::LayerType::Top);
        auto top = GetLogicLayer(src, topIndex);
        if (!top)
            return;
        auto bottomIndex = FindLayerObject(src, CBF::LayerType::Bottom);
        auto bottom = GetLogicLayer(src, bottomIndex);
        if (!bottom)
            return;
        auto getLayerByIndex = [&](uint32_t i) -> CBF::LogicLayer const *
        {
            if (i == multiIndex)
                return multi;
            if (i == topIndex)
                return top;
            if (i == bottomIndex)
                return bottom;
            return nullptr;
        };
        parts.reserve(src.Parts.size());
        pins.reserve(top->Pads.size() + bottom->Pads.size());
        for (CBF::Part const &part : src.Parts)
        {
            if (part.Layer != topIndex && part.Layer != bottomIndex)
                continue;
            auto dstPart = std::make_unique<Part>();
            dstPart->Name(part.Name);
            dstPart->Layer(GetLayerCode(src, part.Layer));
            dstPart->FirstPin(pins.size());
            dstPart->PinCount(part.Pins.size());
            // note 1: assuming pins are sorted by id in ascending order
            // note 2: in Tebo board parts can not have pins on multiple layers
            for (CBF::Pin const &pin : part.Pins)
            {
                auto dstPin = std::make_unique<Pin>();
                dstPin->Name(pin.Name);
                dstPin->Layer(GetLayerCode(src, pin.Layer));
                auto srcLayer = getLayerByIndex(pin.Layer);
                R_ASSERT(srcLayer && "Only multilayer, top and bottom layers are allowed for pins");
                R_ASSERT(pin.Pad < srcLayer->Pads.size());
                auto const &pad = srcLayer->Pads[pin.Pad];
                dstPin->Location(pad.Pos);
                dstPin->Net(pad.Net+1);                
                pins.push_back(std::move(dstPin));
            }
            parts.push_back(std::move(dstPart));
        }
    }

    void Board::Import(CBF::Board const &cbf)
    {
        BuildOutline(cbf);
        netNames = cbf.Nets;
        // XXX: don't include testpoints here
        ProcessLogicLayers(cbf);
    }

    void Board::Save(std::ostream &fs)
    {
        auto const rn = '\n';
        auto outlineBox = Box2i::Empty;
        for (Vector2i const &v : outline)
            outlineBox.Merge(v);
        Vector2i outlineSize = outlineBox.Size();
        StreamWriter w(fs);
        w.OutlineHeight(outlineSize.Y);
        // brdout: n_verts bbox_size
        // vertex1
        // vertex2
        // ...
        int64_t magic = 163LL*(outline[0].X + outline[0].Y);
        magic += 80LL*(outline.size()+1LL);
        magic += 79LL*outlineBox.Height();
        magic += 84LL*outlineBox.Width();
        w.Write(magic, rn);
        w.Write("BRDOUT: ", outline.size() + 1, " ", outlineSize, rn);
        for (size_t i = 0; i < outline.size() + 1; i++)
            w.Write(outline[i % outline.size()], rn);
        w.Write(rn);
        // nets: size
        // index net_name
        w.Write("NETS: ", netNames.size(), rn);
        for (size_t i = 0; i < netNames.size(); i++)
            w.Write(i+1, ' ', netNames[i], rn);
        w.Write(rn);
        // parts: size
        // name bbox.min.x bbox.min.y bbox.max.x bbox.max.y first_pin layer
        w.Write("PARTS: ", parts.size(), rn);
        for (auto const &part : parts)
            w.Write(part.get(), rn);
        w.Write(rn);
        // pins: size
        // pos.x pos.y net_index layer
        w.Write("PINS: ", pins.size(), rn);
        for (auto const &pin : pins)
            w.Write(pin.get(), rn);
        w.Write(rn);
        // nails: size
        // pos.x pos.y net_index layer
        w.Write("NAILS: ", testPoints.size(), rn);
        for (auto const &nail : testPoints)
            w.Write(nail.get(), rn);
    }

    void Board::Export(CBF::Board const &cbf, std::ostream &fs)
    {
        Import(cbf);
        Save(fs);
    }

    static Board::Rep const Frep;

    BoardFormatRep const &Board::Frep() const { return Toptest::Frep; }

    REGISTER_FORMAT(Frep);
}
