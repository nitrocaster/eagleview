// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "ToptestBoardview.hpp"
#include "XMLBrowser.hpp"
#include <map>
#include <unordered_map>
#include <charconv> // std::from_chars

namespace Toptest
{
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
            std::unordered_map<std::string, PadInfo> Pads;
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

        static LibraryInfo ExtractLibraryInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            return {item.String("name")};
        }

        static PartInfo ExtractPartInfo(tinyxml2::XMLBrowser::Proxy &item)
        {
            PartInfo info {};
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

    public:
        Importer(Boardview &brd) : brd(brd) {}

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
            tinyxml2::XMLBrowser browser(xml);
            auto drawing = browser("eagle")("drawing");
            auto board = drawing("board");
    #if 0 // XXX: import board outline
            // <wire x1="-10.7" y1="11.25" x2="-0.5" y2="11.25" width="0" layer="20"/>
            // <wire x1="-11.2" y1="10.75" x2="-10.7" y2="11.25" width="0" layer="20" curve="-90"/>
            for (auto wire = board("plain")("wire"); wire; wire.Next("wire"))
                ;
    #endif
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
                for (auto const &[padName, pad] : pkg.Pads)
                {
                    auto pin = std::make_unique<Pin>();
                    pin->Name(pad.Name);
                    pin->Layer(element.Layer == BoardLayer::Top ? pad.Layer : FlipLayer(pad.Layer));
                    // XXX: apply rotation (element.Rot) to pad.Pos, then add to element.Pos
                    pin->Location(element.Pos + pad.Pos);
                    pin->Net(netNameToIndex[partSignals[element.Name][padName]] + 1);
                    brd.Pins().push_back(std::move(pin));
                }
            }
        }
    };
} // namespace Toptest
