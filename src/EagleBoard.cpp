// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#include "EagleBoard.hpp"
#include "CBF/Board.hpp"
#include "BoardFormatRegistrator.hpp"
#include "Matrix23.hpp"
#include <streambuf> // istreambuf_iterator
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cerrno>

namespace Eagle
{
    static double MillimetersToMils(double v) { return 39.3701*v; }

    static Vector2d MetricVec(double x, double y)
    {
        return {MillimetersToMils(x), MillimetersToMils(y)};
    }

    Board::LibraryInfo Board::ExtractLibraryInfo(XMLProxy &item)
    {
        return {item.String("name")};
    }

    Board::PartInfo Board::ExtractPartInfo(XMLProxy &item)
    {
        PartInfo info{};
        info.Name = item.String("name");
        info.Library = item.String("library");
        info.Package = item.String("package");
        info.Value = item.String("value");
        info.Pos = MetricVec(item.Double("x"), item.Double("y"));
        info.Spin = false;
        info.Mirror = false;
        info.Rot = Angle::FromDegrees(0);
        if (item.HasAttribute("rot"))
        {
            std::string const rotStr = item.String("rot");
            for (size_t i = 0; i < rotStr.size(); i++)
            {
                switch (rotStr[i])
                {
                case 'S':
                    info.Spin = true;
                    break;
                case 'M':
                    info.Mirror = true;
                    break;
                case 'R':
                    errno = 0;
                    double degrees = std::strtod(rotStr.data()+i+1, nullptr);
                    if (errno)
                        throw std::runtime_error("Can't parse 'rot' attribute: invalid angle");
                    info.Rot = Angle::FromDegrees(degrees);
                    i = rotStr.size();
                    break;
                }
            }
        }
        return info;
    }

    Board::SignalInfo Board::ExtractSignalInfo(XMLProxy &item)
    {
        return {item.String("name")};
    }

    Board::ContactRefInfo Board::ExtractContactRef(XMLProxy &item)
    {
        ContactRefInfo cref{};
        cref.Element = item.String("element");
        cref.Pad = item.String("pad");
        return cref;
    }

    Board::PackageInfo Board::ExtractPackageInfo(XMLProxy &item)
    {
        return {item.String("name")};
    }
    
    Board::PadInfo Board::ExtractPadInfo(XMLProxy &item)
    {
        PadInfo pad{};
        pad.Name = item.String("name");
        pad.Pos = MetricVec(item.Double("x"), item.Double("y"));
        if (item.HasAttribute("drill")) // through-hole pad
        {
            double diam;
            if (item.HasAttribute("diameter"))
                diam = item.Double("diameter");
            else
                diam = item.Double("drill");
            pad.Size = MetricVec(diam, diam);
            pad.Layer = LayerId::Multilayer;
        }
        else // smd pad
        {
            pad.Size = MetricVec(item.Double("dx"), item.Double("dy"));
            pad.Layer = LayerId(item.Int32("layer"));
        }
        return pad;
    }

    Board::SectionInfo Board::ExtractSectionInfo(XMLProxy &item)
    {
        SectionInfo section{};
        section.Edge = {
            MetricVec(item.Double("x1"), item.Double("y1")),
            MetricVec(item.Double("x2"), item.Double("y2"))};
        section.Width = item.Double("width");
        section.Layer = LayerId(item.Int32("layer"));
        if (item.HasAttribute("curve"))
            section.Curve = item.Double("curve");
        else
            section.Curve = 0.0;
        return section;
    }

    Board::LayerInfo Board::ExtractLayerInfo(XMLProxy &item)
    {
        LayerInfo layer{};
        layer.Number = static_cast<LayerId>(item.Int32("number"));
        layer.Name = item.String("name");
        layer.Color = item.Int32("color");
        layer.Fill = item.Int32("fill");
        return layer;
    }

    static constexpr double PolyArcTheshold = 8.0;

    template <typename TInserter>
    static void CreatePolyArc(Edge2d edge, double curve, TInserter insert)
    {
        double dist = edge.Length();
        if (dist <= PolyArcTheshold)
        {
            insert(edge);
            return;
        }
        int sign = Sign(curve);
        curve = std::abs(Angle::FromDegrees(curve).Radians());
        Vector2d vec = edge.B - edge.A;
        double h = dist / (2*std::tan(curve/2));
        auto turn = Matrix23::Rotation(Angle::FromDegrees(sign*90.0));
        Vector2d hvec = (turn * vec.Normalize())*h;
        Vector2d center = edge.A + vec/2 + hvec;
        Vector2d rvec = edge.A - center;
        double r = rvec.Length();
        double maxSector = 2*std::asin(PolyArcTheshold/(2*r));
        int sectorCount = int(std::ceil(curve / maxSector));
        double sectorAngle = curve / sectorCount;
        Vector2d prevVertex = edge.A;
        if (sectorCount < 2)
            throw std::runtime_error("Beer from nitrocaster");
        for (int i = 0; i < sectorCount-1; i++)
        {
            turn = Matrix23d::Rotation(Angle::FromRadians(sign*(i+1)*sectorAngle));
            Vector2d v = center + turn*rvec;
            insert({prevVertex, v});
            prevVertex = v;
        }
        insert({prevVertex, edge.B});
    }
    
    void Board::Import(CBF::Board &cbf, std::istream &fs)
    {
        Load(fs);
        Export(cbf);
    }

    void Board::ProcessSection(SectionInfo &&s)
    {
        switch (s.Layer)
        {
        case LayerId::Dimension:
            outline.push_back(std::move(s));
            break;
        }
    }

    void Board::Load(std::istream &fs)
    {
        {
            fs.seekg(0, std::ios::end);
            std::string buf;
            buf.reserve(size_t(fs.tellg()));
            fs.seekg(0, std::ios::beg);
            buf.assign((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
            const std::string xmlPrefix = "<?xml";
            if (buf.compare(0, xmlPrefix.size(), xmlPrefix))
            {
                R_ASSERT(!"Binary Eagle BRD format is not supported. Resave with a newer version and try again.");
            }
            if (src.Parse(buf.c_str()) != tinyxml2::XML_SUCCESS)
            {
                printf("! %s\n", src.ErrorStr());
                R_ASSERT(!"XXX: throw an exception here");
            }
        }
        tinyxml2::XMLBrowser browser(src);
        version = browser("eagle").String("version");
        auto drawing = browser("eagle")("drawing");
        auto brdLayers = drawing("layers");
        for (auto layer = brdLayers.Begin("layer"); layer; layer.Next("layer"))
        {
            auto layerInfo = ExtractLayerInfo(layer);
            R_ASSERT(layers.find(layerInfo.Number) == layers.end());
            layers.emplace(layerInfo.Number, std::move(layerInfo));
        }
        auto board = drawing("board");
        for (auto wire = board("plain").Begin("wire"); wire; wire.Next("wire"))
        {
            auto sectionInfo = ExtractSectionInfo(wire);
            ProcessSection(std::move(sectionInfo));
            //if (!sectionInfo.Curve)
            //{
            //    outlineBuilder.AddEdge(sectionInfo.Edge);
            //    continue;
            //}
            //CreatePolyArc(sectionInfo.Edge, sectionInfo.Curve,
            //    [&](Edge2 e) { outlineBuilder.AddEdge(e); });
        }
        for (auto lib = board("libraries").Begin("library"); lib; lib.Next())
        {
            auto libInfo = ExtractLibraryInfo(lib);
            for (auto pkg = lib("packages").Begin("package"); pkg; pkg.Next())
            {
                auto pkgInfo = ExtractPackageInfo(pkg);
                for (auto pad = pkg.Begin("pad"); pad; pad.Next("pad"))
                {
                    auto padInfo = ExtractPadInfo(pad);
                    pkgInfo.Pads[padInfo.Name] = std::move(padInfo);
                }
                for (auto pad = pkg.Begin("smd"); pad; pad.Next("smd"))
                {
                    auto padInfo = ExtractPadInfo(pad);
                    pkgInfo.Pads[padInfo.Name] = std::move(padInfo);
                }
                libInfo.Packages[pkgInfo.Name] = std::move(pkgInfo);
            }
            libs[libInfo.Name] = std::move(libInfo);
        }
        for (auto part = board("elements").Begin("element"); part; part.Next())
            partInfos.push_back(ExtractPartInfo(part));
        for (auto signal = board("signals").Begin("signal"); signal; signal.Next())
        {
            auto signalInfo = ExtractSignalInfo(signal);
            uint32_t signalIndex = uint32_t(signals.size());
            netNameToIndex[signalInfo.Name] = signalIndex;
            signals.push_back(std::move(signalInfo));
            for (auto cref = signal.Begin("contactref"); cref; cref.Next("contactref"))
            {
                crefCount++;
                auto crefInfo = ExtractContactRef(cref);
                partSignals[crefInfo.Element][crefInfo.Pad] = signalIndex;
            }
        }
    }

    static Board::LayerId operator++(Board::LayerId &id, int)
    {
        auto r = id;
        id = Board::LayerId(int32_t(id) + 1);
        return r;
    }

    static CBF::Color GetColorByIndex(int32_t i)
    {
        static std::array<CBF::Color, 64> const colors =
        {
            0x000000, 0x23238d, 0x238d23, 0x238d8d,
            0x8d2323, 0x8d238d, 0x8d8d23, 0x8d8d8d,
            0x1c1c1c, 0x0000b4, 0x00b400, 0x00b4b4,
            0xb40000, 0xb400b4, 0xb4b400, 0xb4b4b4,
            0xa05000, 0xa07800, 0x285000, 0x505028,
            0x507850, 0x285050, 0x007850, 0x005078,
            0xc87800, 0xc8a028, 0x507800, 0x787850,
            0x78a078, 0x507878, 0x28a078, 0x0078a0,
            0x785078, 0xa07878, 0xa05050, 0x500028,
            0x502850, 0x785050, 0x285078, 0x287878,
            0xa078a0, 0xc8a0a0, 0xc87878, 0x780028,
            0x785078, 0xa07878, 0x5078a0, 0x50a0a0,
            0xc58949, 0x89a429, 0x272727, 0x8d8d8d,
            0x636363, 0x767676, 0x767676, 0x767676,
            0x474747, 0x8d8d8d, 0xb2b2b2, 0xa81d1d,
            0x2da62b, 0xb4b400, 0x2360a2, 0x751eae
        };
        if (0 <= i && i < int32_t(colors.size()))
            return colors[i];
        return colors.back();
    }

    static CBF::LayerType GetLayerRoleById(Board::LayerId id)
    {
        using LayerId = Board::LayerId;
        switch (id)
        {
        case LayerId::Multilayer: return CBF::LayerType::Multilayer;
        case LayerId::Top: return CBF::LayerType::Top;
        case LayerId::Bottom: return CBF::LayerType::Bottom;
        case LayerId::Drills: return CBF::LayerType::Drill;
        case LayerId::Milling: return CBF::LayerType::Drill;
        case LayerId::Dimension: return CBF::LayerType::Route;
        default:
            if (LayerId::Top < id && id < LayerId::Bottom)
                return CBF::LayerType::Signal;
            return CBF::LayerType::Document;
        }
    }
    
    static uint32_t FindLayer(CBF::Board &cbf, CBF::LayerType role)
    {
        auto pred = [&](auto const &p) { return p->Type==role; };
        auto const &it = std::find_if(cbf.Layers.begin(), cbf.Layers.end(), pred);
        R_ASSERT(it!=cbf.Layers.end());
        return uint32_t(std::distance(cbf.Layers.begin(), it));
    }

    static void AddDummyShape(CBF::Board &cbf, uint32_t layerIndex)
    {
        CBF::LogicLayer *layer = *cbf.Layers[layerIndex].get();
        R_ASSERT(layer != nullptr);
        auto shape = new CBF::Round(1);
        shape->Name = "dummy_1mil";
        layer->Shapes.push_back(std::unique_ptr<CBF::Shape>(shape));
    }

    void Board::Export(CBF::Board &cbf)
    {
        // *** nets
        cbf.Nets.reserve(signals.size());
        for (auto const &signal : signals)
            cbf.Nets.push_back(signal.Name);
        // *** layers
        // copper layer count = bottom - multilayer + 1
        // + 1 (dimension)
        cbf.Layers.reserve(int(LayerId::Bottom) - int(LayerId::Multilayer) + 2);
        {
            auto id = LayerId::Multilayer;
            auto layer = new CBF::LogicLayer();
            layer->Type = GetLayerRoleById(id);
            layer->LineColor = 0xc0c0c0;
            layer->PadColor = 0xc0c0c0;
            cbf.Layers.push_back(std::unique_ptr<CBF::Layer>(layer));
        }
        for (auto id = LayerId::Top; id <= LayerId::Bottom; id++)
        {
            auto const it = layers.find(id);
            if (it == layers.end())
                continue;
            LayerInfo const &info = it->second;
            auto layer = new CBF::LogicLayer();
            layer->Name = info.Name;
            layer->Type = GetLayerRoleById(id);
            layer->LineColor = GetColorByIndex(info.Color);
            layer->PadColor = layer->LineColor;
            cbf.Layers.push_back(std::unique_ptr<CBF::Layer>(layer));
        }
        if (auto const it = layers.find(LayerId::Dimension); it != layers.end())
        {
            LayerInfo const &info = it->second;
            auto layer = new CBF::DrillLayer();
            layer->Name = info.Name;
            layer->Type = GetLayerRoleById(LayerId::Dimension);
            layer->LineColor = GetColorByIndex(info.Color);
            layer->PadColor = layer->LineColor;
            for (auto const &section : outline)
            {
                CBF::Slot slot;
                slot.A = section.Edge.A;
                slot.B = section.Edge.B;
                slot.Width = section.Width;
                // XXX: support arc slots (section.Curve)
                slot.Net = -1;
                layer->Slots.push_back(slot);
            }
            cbf.Layers.push_back(std::unique_ptr<CBF::Layer>(layer));
        }
        // *** decals
        for (auto &[libName, lib] : libs)
        {
            for (auto &[pkgName, pkg] : lib.Packages)
            {
                auto bbox = Box2d::Empty;
                for (auto const &[padName, pad] : pkg.Pads)
                    bbox.Merge(Box2d(pad.Size) + pad.Pos);
                CBF::Decal decal;
                decal.Name = pkgName;
                decal.Outline.reserve(4);
                decal.Outline.push_back(bbox.Min);
                decal.Outline.push_back(bbox.Min+bbox.Height());
                decal.Outline.push_back(bbox.Max);
                decal.Outline.push_back(bbox.Max-bbox.Height());
                pkg.Bbox = bbox;
                pkg.Decal = uint32_t(cbf.Decals.size());
                cbf.Decals.push_back(std::move(decal));
            }
        }
        // *** parts
        uint32_t const topLayer = FindLayer(cbf, CBF::LayerType::Top);
        uint32_t const bottomLayer = FindLayer(cbf, CBF::LayerType::Bottom);
        uint32_t const multiLayer = FindLayer(cbf, CBF::LayerType::Multilayer);
        auto translateLayer = [&](LayerId id, bool mirror) -> uint32_t
        {
            switch (id)
            {
            case LayerId::Multilayer:
                return multiLayer;
            case LayerId::Top:
                return mirror ? bottomLayer : topLayer;
            case LayerId::Bottom:
                return mirror ? topLayer : bottomLayer;            
            default:
                R_ASSERT(!"Invalid layer id");
                return 0;
            }
        };
        // add dummy shapes to get around without assigning a real shape to each pad
        AddDummyShape(cbf, multiLayer);
        AddDummyShape(cbf, topLayer);
        AddDummyShape(cbf, bottomLayer);
        for (auto const &part : partInfos)
        {
            auto const &pkg = libs[part.Library].Packages[part.Package];
            CBF::Part cbfPart;
            {
                cbfPart.Name = part.Name;
                cbfPart.Bbox = pkg.Bbox;
                cbfPart.Pos = part.Pos;
                cbfPart.Turn = part.Rot; // top:ccw
                cbfPart.Decal = pkg.Decal;
                cbfPart.Height = 0;
                cbfPart.Value = part.Value;
                cbfPart.Desc = part.Package;
                cbfPart.Layer = translateLayer(LayerId::Top, part.Mirror);
                cbfPart.Pins.reserve(pkg.Pads.size());
            }
            auto transform = Matrix23d::Translation(part.Pos);
            if (part.Mirror)
                transform *= Matrix23d::Rotation(-part.Rot) * Matrix23d::Scaling(Vector2d{-1, 1});
            else
                transform *= Matrix23d::Rotation(part.Rot);
            // for each pad from eagle package:
            // - create a pin and append it to cbf part pins
            // - create a pad and append it to cbf layer
            uint32_t id = 1;
            for (auto const &[padName, pad] : pkg.Pads)
            {
                CBF::Pin cbfPin;
                uint32_t layerIndex = translateLayer(pad.Layer, part.Mirror);
                cbfPin.Layer = layerIndex;
                CBF::LogicLayer *layer = *cbf.Layers[layerIndex].get();
                CBF::Pad cbfPad;
                {
                    cbfPad.Net = partSignals[part.Name][padName];
                    cbfPad.Shape = 0; // XXX: support shapes
                    cbfPad.Pos = transform * pad.Pos;
                    cbfPad.Turn = Angle::FromDegrees(0); // XXX: support pad rotation
                    cbfPad.HoleOffset = Vector2d::Origin; // XXX: support pad holes
                    cbfPad.HoleSize = Vector2d::Origin;
                }
                layer->Pads.push_back(std::move(cbfPad));
                cbfPin.Pad = uint32_t(layer->Pads.size())-1;
                cbfPin.Id = id;
                cbfPin.Name = pad.Name;
                cbfPart.Pins.push_back(std::move(cbfPin));
                id++;
            }
            cbf.Parts.push_back(std::move(cbfPart));
        }
    }

    static Board::Rep const Frep;

    BoardFormatRep const &Board::Frep() const { return Eagle::Frep; }

    REGISTER_FORMAT(Frep);
} // namespace Eagle
