// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "ToptestBoardview.hpp"
#include "XMLBrowser.hpp"
#include "Edge2.hpp"
#include "Matrix23.hpp"
#include "OutlineBuilder.hpp"
#include <map>
#include <unordered_map>
#include <charconv> // std::from_chars
#include <array>
#include <deque>

namespace Toptest
{
    class EagleImporter final
    {
    private:
        Boardview &brd;

        struct PartInfo
        {
            std::string Name;
            std::string Library;
            std::string Package;
            char const *Value; // is extracted from brd, but not currently used
            Vector2 Pos;
            Angle Rot;
            BoardLayer Layer;
            //char const *CRot; // [Mirror][R][45 degrees] -- example : MR45
        };

        struct PadInfo
        {
            std::string Name;
            Vector2 Pos;
            Vector2 Size;
            BoardLayer Layer;
        };

        struct PadComparer
        {
            bool operator()(std::string const &a, std::string const &b) const
            {
                if (a.size() < b.size())
                    return true;
                if (a.size() > b.size())
                    return false;
                for (size_t i = 0; i < a.size(); i++)
                {
                    if (a[i] < b[i])
                        return true;
                    if (a[i] > b[i])
                        return false;
                }
                return false;
            }
        };

        struct PackageInfo
        {
            std::string Name;
            using PadName = std::string;
            std::map<PadName, PadInfo, PadComparer> Pads;
        };

        struct LibraryInfo
        {
            std::string Name;
            using PackageName = std::string;
            std::unordered_map<PackageName, PackageInfo> Packages;
        };

        struct ContactRefInfo
        {
            char const *Element;
            char const *Pad;
        };

        struct SignalInfo
        {
            std::string Name;
        };

        struct SectionInfo
        {
            Edge2 Edge;
            int32_t Layer;
            float Curve;
        };

        static float MillimetersToMils(float v) { return 39.3701f*v; }

        static Vector2 MetricVec(float x, float y)
        { return {MillimetersToMils(x), MillimetersToMils(y)}; }

        static LibraryInfo ExtractLibraryInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            return {item.String("name")};
        }

        static PartInfo ExtractPartInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            PartInfo info{};
            info.Name = item.String("name");
            info.Library = item.String("library");
            info.Package = item.String("package");
            info.Value = item.String("value");
            info.Pos = MetricVec(item.Float("x"), item.Float("y"));
            if (item.HasAttribute("rot"))
            {
                std::string rotStr = item.String("rot");
                size_t pos = rotStr.find('R');
                if (pos == std::string::npos || pos == rotStr.size()-1)
                    throw std::runtime_error("Can't parse 'rot' attribute: R marker not found");
                pos++;
                float degrees = 0;
                auto result = std::from_chars(rotStr.data() + pos, rotStr.data() + rotStr.size(), degrees);
                if (result.ec != std::errc())
                    throw std::runtime_error("Can't parse 'rot' attribute: invalid angle");
                info.Rot = Angle::FromDegrees(degrees);
                info.Layer = rotStr[0] != 'M' ? BoardLayer::Top : BoardLayer::Bottom;
            }
            else
            {
                info.Rot = Angle::FromDegrees(0);
                info.Layer = BoardLayer::Top;
            }  
            return info;
        }

        static SignalInfo ExtractSignalInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            return {item.String("name")};
        }

        static ContactRefInfo ExtractContactRef(tinyxml2::XMLBrowser::Proxy &item)
        {
            ContactRefInfo cref{};
            cref.Element = item.String("element");
            cref.Pad = item.String("pad");
            return cref;
        }

        static PackageInfo ExtractPackageInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            return {item.String("name")};
        }

        static BoardLayer DecodeEagleLayer(int id)
        {
            switch (id)
            {
            case 1:
                return BoardLayer::Top;
            case 16:
                return BoardLayer::Bottom;
            default:
                return BoardLayer::Multilayer;
            }
        }

        static PadInfo ExtractPadInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            PadInfo pad{};
            pad.Name = item.String("name");
            pad.Pos = MetricVec(item.Float("x"), item.Float("y"));
            if (item.HasAttribute("drill")) // through-hole pad
            {
                float diam;
                if (item.HasAttribute("diameter"))
                    diam = item.Float("diameter");
                else
                    diam = item.Float("drill");
                pad.Size = MetricVec(diam, diam);
                pad.Layer = BoardLayer::Multilayer;
            }
            else // smd pad
            {
                pad.Size = MetricVec(item.Float("dx"), item.Float("dy"));
                pad.Layer = DecodeEagleLayer(item.Int32("layer"));
            }
            return pad;
        }

        static BoardLayer FlipLayer(BoardLayer layer)
        {
            switch (layer)
            {
            case BoardLayer::Top:
                return BoardLayer::Bottom;
            case BoardLayer::Bottom:
                return BoardLayer::Top;
            case BoardLayer::Multilayer:
            default:
                return BoardLayer::Multilayer;
            }
        }

        static SectionInfo ExtractSectionInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            SectionInfo section{};
            section.Edge = {MetricVec(item.Float("x1"), item.Float("y1")), MetricVec(item.Float("x2"), item.Float("y2"))};
            section.Layer = item.Int32("layer");
            if (item.HasAttribute("curve"))
                section.Curve = item.Float("curve");
            else
                section.Curve = 0.0f;
            return section;
        }

        static constexpr float PolyArcTheshold = 8.0f;

        template <typename TInserter>
        static void CreatePolyArc(Edge2 edge, float curve, TInserter insert)
        {
            float dist = edge.Length();
            if (dist <= PolyArcTheshold)
            {
                insert(edge);
                return;
            }
            int sign = Sign(curve);
            curve = std::fabs(Angle::FromDegrees(curve).Radians());
            Vector2 vec = edge.B - edge.A;
            float h = dist / (2*std::tanf(curve/2));
            auto turn = Matrix23::Rotation(Angle::FromDegrees(sign*90.0f));
            Vector2 hvec = (turn * vec.Normalize())*h;
            Vector2 center = edge.A + vec/2 + hvec;
            Vector2 rvec = edge.A - center;
            float r = rvec.Length();
            float maxSector = 2*std::asinf(PolyArcTheshold/(2*r));
            int sectorCount = int(std::ceilf(curve / maxSector));
            float sectorAngle = curve / sectorCount;            
            Vector2 prevVertex = edge.A;
            if (sectorCount < 2)
                throw std::runtime_error("Beer from nitrocaster");
            for (int i = 0; i < sectorCount-1; i++)
            {
                turn = Matrix23::Rotation(Angle::FromRadians(sign*(i+1)*sectorAngle));
                Vector2 v = center + turn*rvec;
                insert({prevVertex, v});
                prevVertex = v;
            }
            insert({prevVertex, edge.B});
        }

    public:
        EagleImporter(Boardview &brd) :
            brd(brd)
        {}

        void Import(tinyxml2::XMLDocument &xml)
        {
            std::vector<PartInfo> partInfos;
            std::unordered_map<std::string, LibraryInfo> libs;
            std::vector<SignalInfo> signals;
            using PadName = std::string;
            using SignalName = std::string;
            using SignalMap = std::map<PadName, SignalName>;
            using ElementName = std::string;
            std::unordered_map<ElementName, SignalMap> partSignals;
            std::unordered_map<SignalName, size_t> netNameToIndex;
            OutlineBuilder outlineBuilder;
            tinyxml2::XMLBrowser browser(xml);
            auto drawing = browser("eagle")("drawing");
            auto board = drawing("board");
            for (auto wire = board("plain").Begin("wire"); wire; wire.Next("wire"))
            {
                auto sectionInfo = ExtractSectionInfo(wire);
                if (sectionInfo.Layer != 20) // outline must be in layer 20
                    continue;
                if (!sectionInfo.Curve)
                {
                    outlineBuilder.AddEdge(sectionInfo.Edge);
                    continue;
                }
                CreatePolyArc(sectionInfo.Edge, sectionInfo.Curve,
                    [&](Edge2 e) { outlineBuilder.AddEdge(e); });
            }
            outlineBuilder.Build(brd.Outline());
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
            size_t crefs = 0;
            for (auto signal = board("signals").Begin("signal"); signal; signal.Next())
            {
                auto signalInfo = ExtractSignalInfo(signal);
                brd.Nets().push_back(signalInfo.Name);
                netNameToIndex[signalInfo.Name] = brd.Nets().size() - 1;
                for (auto cref = signal.Begin("contactref"); cref; cref.Next("contactref"))
                {
                    crefs++;
                    auto crefInfo = ExtractContactRef(cref);
                    partSignals[crefInfo.Element][crefInfo.Pad] = signalInfo.Name;
                }
            }
            // *** output format ***
            // nets: size
            // index net_name
            // 
            // parts: size
            // name bbox.min.x bbox.min.y bbox.max.x bbox.max.y first_pin layer
            //
            // pins: size
            // pos.x pos.y net_index layer
            brd.Parts().reserve(partInfos.size());
            brd.Pins().reserve(crefs);
            for (auto const &element : partInfos)
            {
                PackageInfo const &pkg = libs[element.Library].Packages[element.Package];
                if (pkg.Pads.empty())
                    continue;
                auto part = std::make_unique<Part>();
                part->Name(element.Name);
                part->Layer(element.Layer);
                part->FirstPin(brd.Pins().size());
                part->PinCount(pkg.Pads.size());
                brd.Parts().push_back(std::move(part));
                auto elementTransform = Matrix23::Translation(element.Pos);
                if (element.Layer == BoardLayer::Bottom)
                    elementTransform *= Matrix23::Rotation(-element.Rot) * Matrix23::Scaling(Vector2{-1, 1});
                else
                    elementTransform *= Matrix23::Rotation(element.Rot);
                for (auto const &[padName, pad] : pkg.Pads)
                {
                    auto pin = std::make_unique<Pin>();
                    pin->Name(pad.Name);
                    pin->Layer(element.Layer == BoardLayer::Top ? pad.Layer : FlipLayer(pad.Layer));
                    pin->Location(elementTransform * pad.Pos);
                    pin->Net(netNameToIndex[partSignals[element.Name][padName]] + 1);
                    brd.Pins().push_back(std::move(pin));
                }
            }
        }
    };
} // namespace Toptest
