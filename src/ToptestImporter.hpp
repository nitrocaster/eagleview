// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "ToptestBoardview.hpp"
#include "XMLBrowser.hpp"
#include "Edge2.hpp"
#include "Matrix23.hpp"
#include <map>
#include <unordered_map>
#include <charconv> // std::from_chars
#include <array>

namespace Toptest
{
    class OutlineBuilder final
    {
    private:
        struct Index
        {
            static constexpr size_t InvalidValue = size_t(-1);
            size_t Value;
            
            constexpr Index(size_t value = InvalidValue) :
                Value(value)
            {}

            constexpr operator size_t() const
            { return Value; }

            constexpr bool Valid() const
            { return Value != InvalidValue; }
        };
        
        struct VertexData
        {
            Vector2 V;
            Index Self;
            std::array<Index, 2> Neighbors{};

            VertexData() = default;

            VertexData(Vector2 v, Index self) :
                V(v),
                Self(self)
            {}

            bool AddNeighbor(Index i)
            {
                if (!Neighbors[0].Valid())
                {
                    Neighbors[0] = i;
                    return true;
                }
                if (!Neighbors[1].Valid())
                {
                    Neighbors[1] = i;
                    return true;
                }
                return false;
            }
        };
        
        struct Vector2Hasher
        {
            uint64_t operator()(Vector2 const &vec) const noexcept
            {
                return uint64_t(double(vec.X)*73856093) ^ uint64_t(double(vec.Y)*83492791);
            }
        };

        std::vector<VertexData> vertices;
        std::unordered_map<Vector2, Index, Vector2Hasher> vertexSet;
        using Loop = std::vector<Index>;
        std::vector<Loop> loops;

        Index FindVertex(Vector2 v)
        {
            Index &index = vertexSet[v];
            if (!index.Valid())
            {
                index = vertices.size();
                VertexData data(v, index);
                vertices.push_back(data);
            }
            return index;
        }

        std::string VectorToString(Vector2 v)
        { return std::to_string(v.X) + ", " + std::to_string(v.Y); }

        Index NextVertex(VertexData const &current, Index prev)
        {
            auto pred = [&](Index next) { return next.Valid() && next != prev; };
            auto next = std::find_if(begin(current.Neighbors), end(current.Neighbors), pred);
            if (next == current.Neighbors.end())
                return Index::InvalidValue;
            else
                return *next;
        }

        Loop NextLoop(std::vector<bool> &visited, std::vector<bool>::iterator &it)
        {
            Loop loop;
            for (Index index = it - visited.begin(), prev = index; index.Valid() && !visited[index];)
            {
                VertexData const &vd = vertices[index];
                loop.push_back(index);
                visited[index] = true;
                if (vd.Self != index)
                    throw std::runtime_error("Index from VertexData must be equal to the computed index");
                index = NextVertex(vd, prev);
                prev = vd.Self;
            }
            it = visited.begin() + loop.back() + 1;
            return loop;
        }

    public:
        void AddEdge(Edge2 edge)
        {
            Index ia = FindVertex(edge.A);
            Index ib = FindVertex(edge.B);
            VertexData &a = vertices[ia];
            VertexData &b = vertices[ib];
            if (!a.AddNeighbor(b.Self))
            {
                auto msg = std::string("Vertex (") + VectorToString(edge.A) + ") is shared by more than 2 edges";
                throw std::runtime_error(msg);
            }
            if (!b.AddNeighbor(a.Self))
            {
                auto msg = std::string("Vertex (") + VectorToString(edge.B) + ") is shared by more than 2 edges";
                throw std::runtime_error(msg);
            }
        }

        void Build(std::vector<Vector2> &output)
        {
            if (vertices.empty())
                return;
            std::vector<bool> visited;
            visited.resize(vertices.size());
            std::fill(begin(visited), end(visited), false);
            for (auto it = visited.begin(); it != visited.end(); it = std::find(it, visited.end(), false))
                loops.push_back(NextLoop(visited, it));
            // XXX: return one loop which encloses other loops
            for (auto i : loops.front())
                output.push_back(vertices[i].V);
        }
    };

    class Importer final
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

        struct PackageInfo
        {
            std::string Name;
            using PadName = std::string;
            std::unordered_map<PadName, PadInfo> Pads;
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
            info.Pos = {item.Float("x"), item.Float("y")};
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
            pad.Pos = {item.Float("x"), item.Float("y")};
            if (item.HasAttribute("diameter")) // through-hole pad
            {
                float diam = item.Float("diameter");
                pad.Size = {diam, diam};
                pad.Layer = BoardLayer::Multilayer;
            }
            else // smd pad
            {
                pad.Size = {item.Float("dx"), item.Float("dy")};
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
            section.Edge = {Vector2(item.Float("x1"), item.Float("y1")), Vector2(item.Float("x2"), item.Float("y2"))};
            section.Layer = item.Int32("layer");
            if (item.HasAttribute("curve"))
                section.Curve = item.Float("curve");
            else
                section.Curve = 0.0f;
            return section;
        }

    public:
        Importer(Boardview &brd) :
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
            float const reductionThreshold = 1.0f;
            tinyxml2::XMLBrowser browser(xml);
            auto drawing = browser("eagle")("drawing");
            auto board = drawing("board");
            for (auto wire = board("plain")("wire"); wire; wire.Next("wire"))
            {
                auto sectionInfo = ExtractSectionInfo(wire);
                if (sectionInfo.Layer != 20) // outline must be in layer 20
                    continue;
                if (sectionInfo.Curve == 0 || sectionInfo.Edge.SqrLength() < reductionThreshold)
                {
                    outlineBuilder.AddEdge(sectionInfo.Edge);
                    continue;
                }
                // XXX: generate intermediate vertices
                // XXX: add edges to OutlineBuilder
            }
            outlineBuilder.Build(brd.Outline());
            for (auto lib = board("libraries")("library"); lib; lib.Next())
            {
                auto libInfo = ExtractLibraryInfo(lib);
                for (auto pkg = lib("packages")("package"); pkg; pkg.Next())
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
            for (auto part = board("elements")("element"); part; part.Next())
                partInfos.push_back(ExtractPartInfo(part));
            size_t crefs = 0;
            for (auto signal = board("signals")("signal"); signal; signal.Next())
            {
                auto signalInfo = ExtractSignalInfo(signal);
                brd.Nets().push_back(signalInfo.Name);
                netNameToIndex[signalInfo.Name] = brd.Nets().size() - 1;
                for (auto cref = signal("contactref"); cref; cref.Next("contactref"))
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
                auto part = std::make_unique<Part>();
                part->Name(element.Name);
                part->Layer(element.Layer);
                part->FirstPin(brd.Pins().size());
                part->PinCount(pkg.Pads.size());
                brd.Parts().push_back(std::move(part));
                auto elementTransform = Matrix23::Translation(element.Pos) * Matrix23::Rotation(element.Rot);
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
