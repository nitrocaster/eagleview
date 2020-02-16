// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "CBF/Board.hpp"
#include "BoardFormat.hpp"
#include "XMLBrowser.hpp"
#include "Edge2.hpp"
#include "Matrix23.hpp"
#include <map>
#include <unordered_map>
#include <charconv> // std::from_chars

namespace Eagle
{
    class Board : public BoardFormat
    {
    public:
        struct PartInfo
        {
            std::string Name;
            std::string Library;
            std::string Package;
            std::string Value;
            Vector2d Pos;
            Angle Rot;
            bool Mirror;
            bool Spin;
            //char const *CRot; // [Mirror][R][45 degrees] -- example : MR45
        };

        enum class LayerId : int32_t
        {
            Multilayer = 0,
            Top = 1,
            // 2..15 internal layers
            Bottom = 16,
            Pads = 17,
            Vias = 18,
            Dimension = 20,
            Drills = 44,
            Holes = 45,
            Milling = 46,
        };

        struct PadInfo
        {
            std::string Name;
            Vector2d Pos; // local position
            Vector2d Size;
            LayerId Layer; // 0 : multilayer
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
            // converter helpers
            uint32_t Decal = uint32_t(-1); // index of Decal in Board::Decals
            Box2d Bbox;
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

        struct SectionInfo
        {
            Edge2d Edge;
            double Width;
            LayerId Layer;
            double Curve;
        };

        struct SignalInfo
        {
            std::string Name;
        };

        struct LayerInfo
        {
            LayerId Number;
            std::string Name;
            int32_t Color;
            int32_t Fill;
        };

    private:
        tinyxml2::XMLDocument src;
        std::string version;
        std::unordered_map<LayerId, LayerInfo> layers;
        std::vector<SectionInfo> outline;
        std::vector<PartInfo> partInfos;
        std::unordered_map<std::string, LibraryInfo> libs;
        std::vector<SignalInfo> signals;
        size_t crefCount = 0;
        using PadName = std::string;
        using SignalName = std::string;
        using SignalMap = std::map<PadName, uint32_t>;
        using ElementName = std::string;
        std::unordered_map<ElementName, SignalMap> partSignals;
        std::unordered_map<SignalName, size_t> netNameToIndex;

    public:
        using XMLProxy = tinyxml2::XMLBrowser::Proxy;

        static LibraryInfo ExtractLibraryInfo(XMLProxy &item);
        static PartInfo ExtractPartInfo(XMLProxy &item);
        static SignalInfo ExtractSignalInfo(XMLProxy &item);
        static ContactRefInfo ExtractContactRef(XMLProxy &item);
        static PackageInfo ExtractPackageInfo(XMLProxy &item);
        static PadInfo ExtractPadInfo(XMLProxy &item);
        static SectionInfo ExtractSectionInfo(XMLProxy &item);
        static LayerInfo ExtractLayerInfo(XMLProxy &item);

        class Rep : public BoardFormatRep
        {
        public:
            Rep() : BoardFormatRep((Board *)0)
            {}
            virtual char const *Tag() const override { return "eagle"; }
            virtual char const *Desc() const override { return "Autodesk EAGLE board (*.BRD)"; }
            virtual bool CanImport() const override { return true; }            
        };

        virtual void Import(CBF::Board &cbf, std::istream &fs) override;        
        virtual BoardFormatRep const &Frep() const override;

    private:
        void ProcessSection(SectionInfo &&s);
        void Load(std::istream &fs);
        void Export(CBF::Board &cbf);
    };
} // namespace Eagle
