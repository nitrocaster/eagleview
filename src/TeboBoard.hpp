// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "Fixed32.hpp"
#include "StreamReader.hpp"
#include <istream> // std::istream
#include <optional>
#include <vector>

namespace Tebo
{
    struct Box2S
    {
        Vector2S Min, Max;
    };

    inline void ValidatePos(Vector2S v) {}

    enum class LayerType : uint32_t
    {
        Document = 0,
        Top = 1,
        Bottom = 2,
        Signal = 3,
        Plane = 4, // PowerGround
        SolderTop = 5,
        SolderBottom = 6,
        SilkTop = 7,
        SilkBottom = 8,
        PasteTop = 9,
        PasteBottom = 10,
        Drill = 11,
        Roul = 12
    };

    enum class PrimitiveType
    {
        Line,
        Arc,
        Surface
    };

    enum class ShapeType
    {
        Round = 0,
        Rect = 1,
        Oblong = 3,
        Poly = 5,
    };

    struct Shape abstract
    {
        ShapeType Type;
        Vector2S Size;
        std::string Name;

        Shape(ShapeType shapeType, Vector2S shapeSize)
        {
            Type = shapeType;
            Size = shapeSize;
        }

        virtual ~Shape() = default;

        static std::unique_ptr<Shape> Load(StreamReader &r);
    };

    struct Round : public Shape
    {
        Round(Vector2S shapeSize) :
            Shape(ShapeType::Round, shapeSize)
        {}
    };

    struct Rect : public Shape
    {
        float Param = 0.0f;

        Rect(Vector2S shapeSize, float param) :
            Shape(ShapeType::Rect, shapeSize),
            Param(param)
        {}
    };
    
    struct Poly : public Shape
    {
        struct Line
        {
            int32_t Param1, Param2, Param3; // 1, 0, 0
            Vector2S Start, End;
            Fixed32 Thickness;

            void Load(StreamReader &r)
            {
                Param1 = r.ReadU32();
                R_ASSERT(Param1 == 1);
                Param2 = r.ReadU32();
                R_ASSERT(Param2 == 0);
                Param3 = r.ReadU32();
                R_ASSERT(Param3 == 0);
                Start = r.ReadVec2S();
                End = r.ReadVec2S();
                Thickness = r.ReadS32();
            }
        };

        Box2S BBox;
        std::vector<Poly::Line> Lines;
        int32_t Flags[3] = {};
        std::vector<Vector2S> Vertices;

        Poly(Vector2S shapeSize, std::string const &shapeName) :
            Shape(ShapeType::Poly, shapeSize)
        {
            Name = shapeName;
        }
    };

    struct Oblong : public Shape
    {
    private:
        int32_t radius;

    public:
        Oblong(Vector2S shapeSize, int32_t shapeRadius) :
            Shape(ShapeType::Oblong, shapeSize),
            radius(shapeRadius)
        {}
    };

    inline std::unique_ptr<Shape> Shape::Load(StreamReader &r)
    {
        {
            uint32_t one = r.ReadU32();
            R_ASSERT(one == 1);
        }
        auto size = r.ReadVec2S();
        auto type = static_cast<ShapeType>(r.ReadU32());
        switch (type)
        {
        case ShapeType::Round:
        {
            auto v = r.ReadVec2S();
            R_ASSERT(!v.X && !v.Y);
            return std::make_unique<Round>(size);
        }
        case ShapeType::Rect:
        {
            auto param = r.ReadFloat();
            auto z = r.ReadS32();
            R_ASSERT(z == 0);
            return std::make_unique<Rect>(size, param);
        }
        case ShapeType::Poly:
        {
            auto v = r.ReadU32();
            R_ASSERT(v == 0);
            auto name = r.ReadString255();
            auto shape = std::make_unique<Poly>(size, name);
            shape->BBox.Min = r.ReadVec2S();
            shape->BBox.Max = r.ReadVec2S();
            auto subObjCount = r.ReadU32();
            for (uint32_t i = 0; i < subObjCount; i++)
            {
                auto subObjType = r.ReadU32();
                switch (subObjType)
                {
                case 2: // poly
                {
                    R_ASSERT(shape->Vertices.empty());
                    r.Read(shape->Flags, 3);
                    uint32_t vertexCount = r.ReadU32();
                    shape->Vertices.reserve(vertexCount);
                    for (uint32_t i = 0; i < vertexCount; i++)
                        shape->Vertices.push_back(r.ReadVec2S());
                    break;
                }
                case 5: // line
                {
                    shape->Lines.emplace_back().Load(r);
                    break;
                }
                default:
                    R_ASSERT(!"Unrecognized subobject type");
                    break;
                }
            }
            return shape;
        }
        case ShapeType::Oblong:
        {
            auto v = r.ReadU32();
            R_ASSERT(v == 0);
            auto radius = r.ReadS32();
            return std::make_unique<Oblong>(size, radius);
        }
        default:
            R_ASSERT(!"Unrecognized shape type");
            return nullptr;
        }
    }

    struct Pad
    {
        Shape *Shape;
        int32_t Net;
        uint32_t DCode;
        Vector2S Pos;
        bool IsExposed;
        bool IsCopper;
        uint8_t TestpointParam; // 0 - smd pin, 1 - accessible, 2 - mask
        // extensions
        bool IsSomething = false;
        struct TestPointData
        {
            uint8_t Data12[12] = {};
        } TestPoint;
        struct ExposedData
        {
            Vector2S Min = {};
            Vector2S Max = {};
        } Exposed;
        bool HasHole = false;
        uint8_t TailParam = 0;
        struct HoleData
        {
            uint8_t Data7[7] = {};
            Vector2S Size = {};
            uint8_t Param = 0;
        } Hole;
    };

    struct Primitive abstract
    {
        int32_t Net;
        PrimitiveType Type;
        uint32_t DCode;

        Primitive(PrimitiveType type)
        { Type = type; }

        virtual ~Primitive() = default;
    };

    struct Line : public Primitive
    {
        Vector2S StartPos, EndPos;

        Line() : Primitive(PrimitiveType::Line)
        {}
    };

    struct Arc : public Primitive
    {
        Vector2S Pos;
        Fixed32 Radius;
        float StartAngle, SweepAngle;

        Arc() : Primitive(PrimitiveType::Arc)
        {}
    };

    struct Cutout
    {
        uint32_t Tag;
        uint32_t EdgeCount;
        std::vector<Vector2S> Vertices;
    };

    struct Surface : public Primitive
    {
        uint32_t EdgeCount;
        Fixed32 LineWidth;
        uint32_t VoidCount;
        std::vector<Vector2S> Vertices;
        std::vector<Cutout> Voids;
        uint32_t VoidFlags = 0;

        Surface() : Primitive(PrimitiveType::Surface)
        {
            DCode = 0;
        }
    };

    inline void DecodeString(std::string &s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            char c = s[i];
            if ('a' <= c && c <= 'j')
            {
                char x = c - i%3 - 4;
                if (x < 'a')
                    x += 10;
                s[i] = 154 - x;
                continue;
            }
            if ('k' <= c && c <= 'z')
            {
                char x = c - i%10 - 5;
                c = x;
                if (x < 'k')
                    c = x + 16;
                s[i] = c;
                continue;
            }
            if ('A' <= c && c <= 'Z')
            {
                char x = c + i%10 + 5;
                c = x;
                if (x > 'Z')
                    c = x - 26;
                s[i] = c;
                continue;
            }
            if ('0' <= c && c <= '9')
            {
                char x = c + i%3 + 4;
                if (x > '9')
                    x -= 10;
                s[i] = x + 49;
                continue;
            }
        }
    }

    struct TvwHeader
    {
        std::string Type;
        uint32_t Const1; // = 1
        std::string Customer;
        uint8_t Const2; // = 0
        std::string Date;
        uint8_t Const3[3]; // = {0, 0, 0}
        uint32_t Size1;
        uint32_t Size2;
        uint32_t Size3;
        uint32_t LayerCount;

        void Load(StreamReader &r)
        {
            Type = r.ReadString255();
            DecodeString(Type);
            Const1 = r.ReadU32();
            Customer = r.ReadString255();
            DecodeString(Customer);
            Const2 = r.ReadU8();
            Date = r.ReadString255();
            DecodeString(Date);
            r.Read(Const3, 3);
            Size1 = r.ReadU32();
            Size2 = r.ReadU32();
            Size3 = r.ReadU32();
            LayerCount = r.ReadU32();
        }
    };

    enum class ObjectType : uint32_t
    {
        Undefined = 0,
        Through = 1,
        Logic = 3
    };

    inline char const *LayerTypeToString(LayerType type)
    {
        switch (type)
        {
        case LayerType::Document: return "Document";
        case LayerType::Top: return "Top";
        case LayerType::Bottom: return "Bottom";
        case LayerType::Signal: return "Signal";
        case LayerType::Plane: return "Plane";
        case LayerType::SolderTop: return "SolderTop";
        case LayerType::SolderBottom: return "SolderBottom";
        case LayerType::SilkTop: return "SilkTop";
        case LayerType::SilkBottom: return "SilkBottom";
        case LayerType::PasteTop: return "PasteTop";
        case LayerType::PasteBottom: return "PasteBottom";
        case LayerType::Drill: return "Drill";
        case LayerType::Roul: return "Roul";
        default: return "unknown";
        }
    }

    struct Object
    {
        ObjectType ObjType; // 3 or 1
        uint32_t Magic[2]; // 2, 1
        std::string Name;
        std::string InitialName;
        std::string InitialPath;
        LayerType Type;
        uint32_t PadColor;
        uint32_t LineColor;

        static ObjectType Detect(StreamReader &r)
        {
            const uint32_t maxSkips = 4;
            for (uint32_t i = 0; i < maxSkips; i++)
            {
                uint32_t type = r.ReadU32();
                if (!type)
                    continue;
                return static_cast<ObjectType>(type);
            }
            return ObjectType::Undefined;
        }

        Object(ObjectType objType)
        {
            R_ASSERT(objType == ObjectType::Logic || objType == ObjectType::Through);
            ObjType = objType;
        }

        virtual ~Object() = default;

        virtual void Load(StreamReader &r)
        {
            // 3, 2, 1 # logic layer
            // 1, 2, 1 # thru layer (from 0 to 0)
            uint32_t pos = static_cast<uint32_t>(r.Tell());
            r.Read(Magic, 2);
            R_ASSERT(Magic[0] == 2 && Magic[1] == 1);
            Name = r.ReadString255();
            InitialName = r.ReadString255();
            InitialPath = r.ReadString255();
            Type = static_cast<LayerType>(r.ReadU32());
            PadColor = r.ReadU32();
            LineColor = r.ReadU32();
            printf("- loading object name[%s] type[%s] addr[0x%08X]\n",
                Name.c_str(), LayerTypeToString(Type), pos);
        }
    };

    struct TestPoint
    {
        bool Flag1;
        int32_t P1, Handle, P2;
        int32_t P3;
        Vector2S Pos;
        int32_t P4;
        bool Flag2;
        int32_t P5, P6;
        int32_t N;

        void Load(StreamReader &r)
        {
            Flag1 = r.ReadBool8();
            P1 = r.ReadS32();
            Handle = r.ReadS32();
            P2 = r.ReadS32();
            P3 = r.ReadS32();
            Pos = r.ReadVec2S();
            ValidatePos(Pos);
            P4 = r.ReadS32();
            Flag2 = r.ReadBool8();
            P5 = r.ReadS32();
            P6 = r.ReadS32();
            N = r.ReadS32();
        }
    };

    struct TestPoint2
    {
        uint32_t P1, Handle, P2;
        Vector2S Pos;
        Vector2S Pos1, Pos2;
        bool Flag1, Flag2, Flag3;
        uint32_t Nail;
        int32_t Param;
        bool Flag4, Flag5, Flag6;
        int32_t N;

        void Load(StreamReader &r)
        {
            P1 = r.ReadS32();
            Handle = r.ReadS32();
            P2 = r.ReadS32();
            Pos = r.ReadVec2S();
            ValidatePos(Pos);
            Pos1 = r.ReadVec2S();
            Pos2 = r.ReadVec2S();
            Flag1 = r.ReadBool8();
            Flag2 = r.ReadBool8();
            Flag3 = r.ReadBool8();
            Nail = r.ReadU32();
            Param = r.ReadS32();
            Flag4 = r.ReadBool8();
            Flag5 = r.ReadBool8();
            Flag6 = r.ReadBool8();
            N = r.ReadS32();
        }
    };

    struct TestNode
    {
        uint32_t Current, Next;
        bool Flag;

        void Load(StreamReader &r)
        {
            Current = r.ReadU32();
            Next = r.ReadU32();
            Flag = r.ReadBool8();
        }
    };

    struct UnknownItem
    {
        std::string Name;
        Vector2S Pos;
        int32_t Z1;
        int32_t Param1, Param2, Param3;
        int32_t Z2, Z3;
        bool Flags[3];
        uint32_t Param4;

        void Load(StreamReader &r)
        {
            Name = r.ReadString255();
            Pos = r.ReadVec2S();
            Z1 = r.ReadS32();
            Param1 = r.ReadS32();
            Param2 = r.ReadS32();
            Param3 = r.ReadS32();
            Z2 = r.ReadS32();
            Z3 = r.ReadS32();
            r.Read(Flags, 3);
            Param4 = r.ReadS32();
        }
    };

    struct LogicLayer : public Object
    {
        std::vector<std::unique_ptr<Shape>> Shapes;
        std::vector<Pad> Pads;
        std::vector<Line> Lines;
        std::vector<Arc> Arcs;
        std::vector<Surface> Surfaces;
        uint32_t UnknownItemCount;
        uint32_t UnknownItemsParam;
        std::vector<UnknownItem> UnknownItems;
        uint32_t TpCount;
        std::vector<TestPoint> TestPoints;
        uint32_t TPS2Size;
        uint32_t TPS2Param;
        std::vector<TestPoint2> TestPoints2; // first tps
        uint32_t TPS3Size;
        uint32_t TPS3Param;
        std::vector<TestPoint2> TestPoints3; // assistant tps
        uint32_t TestSequenceSize;
        uint32_t TestSequenceParam;
        std::vector<TestNode> TestSequence;

        LogicLayer() : Object(ObjectType::Logic)
        {}

        void LoadShapes(StreamReader &r)
        {
            // this is actually max dcode, not a 'count'
            uint32_t shapeCount = r.ReadU32();
            if (!shapeCount)
                return;
            R_ASSERT(shapeCount >= 10);
            shapeCount -= 10;
            for (uint32_t i = 0; i < shapeCount; i++)
            {
                std::unique_ptr<Shape> shape = Shape::Load(r);
                R_ASSERT(shape != nullptr);
                Shapes.push_back(std::move(shape));
            }
        }

        void LoadPads(StreamReader &r)
        {
            uint32_t instanceCount = r.ReadU32();
            if (!instanceCount)
                return;
            {
                uint32_t two = r.ReadU32();
                R_ASSERT(two == 2);
            }
            Pads.reserve(instanceCount);
            for (uint32_t i = 0; i < instanceCount; i++)
            {
                Pad obj;
                obj.Net = r.ReadS32();
                obj.DCode = r.ReadU32();
                obj.Pos = r.ReadVec2S();
                obj.IsExposed = r.ReadBool8();
                obj.IsCopper = r.ReadBool8();
                obj.TestpointParam = r.ReadU8();
                R_ASSERT(obj.DCode-10 < Shapes.size());
                obj.Shape = Shapes[obj.DCode-10].get();
                if (obj.IsCopper)
                {
                    obj.IsSomething = r.ReadBool8();
                    if (obj.TestpointParam == 1)
                        r.Read(obj.TestPoint.Data12, 12);
                    if (obj.IsExposed || obj.IsSomething)
                    {
                        obj.Exposed.Min = r.ReadVec2S();
                        obj.Exposed.Max = r.ReadVec2S();
                    }
                    obj.HasHole = r.ReadBool8();
                    obj.TailParam = r.ReadU8();
                    if (obj.HasHole)
                    {
                        r.Read(obj.Hole.Data7, 7);
                        obj.Hole.Size = r.ReadVec2S();
                        obj.Hole.Param = r.ReadU8();
                    }
                }
                else // not copper
                {
                    R_ASSERT(!obj.IsExposed && obj.TestpointParam != 1); // not expected
                    // no exposed copper => no extra data
                }
                Pads.push_back(std::move(obj));
            }
        }

        void LoadLines(StreamReader &r)
        {
            uint32_t instanceCount = r.ReadU32();
            if (!instanceCount)
                return;
            {
                uint32_t zero = r.ReadU32();
                R_ASSERT(!zero);
            }
            Lines.reserve(instanceCount);
            for (uint32_t i = 0; i < instanceCount; i++)
            {
                Line obj;
                obj.Net = r.ReadS32();
                obj.DCode = r.ReadU32();
                R_ASSERT(obj.DCode-10 < Shapes.size());
                obj.StartPos = r.ReadVec2S();
                obj.EndPos = r.ReadVec2S();
                Lines.push_back(std::move(obj));
            }
        }

        void LoadArcs(StreamReader &r)
        {
            uint32_t instanceCount = r.ReadU32();
            if (!instanceCount)
                return;
            {
                uint32_t zero = r.ReadU32();
                R_ASSERT(!zero);
            }
            Arcs.reserve(instanceCount);
            for (uint32_t i = 0; i < instanceCount; i++)
            {
                Arc obj;
                obj.Net = r.ReadS32();
                obj.DCode = r.ReadU32();
                R_ASSERT(obj.DCode-10 < Shapes.size());
                obj.Pos = r.ReadVec2S();
                obj.Radius = r.ReadS32();
                obj.StartAngle = r.ReadFloat();
                obj.SweepAngle = r.ReadFloat();
                Arcs.push_back(std::move(obj));
            }
        }

        void LoadSurfaces(StreamReader &r)
        {
            uint32_t instanceCount = r.ReadU32();
            if (!instanceCount)
                return;
            {
                uint32_t two = r.ReadU32();
                R_ASSERT(two == 2);
            }
            Surfaces.reserve(instanceCount);
            for (uint32_t i = 0; i < instanceCount; i++)
            {
                Surface obj;
                obj.Net = r.ReadS32();
                obj.EdgeCount = r.ReadU32();
                obj.Vertices.reserve(obj.EdgeCount);
                for (uint32_t vi = 0; vi < obj.EdgeCount; vi++)
                    obj.Vertices.push_back(r.ReadVec2S());
                obj.LineWidth = r.ReadS32();
                obj.VoidCount = r.ReadU32();
                if (obj.VoidCount)
                {
                    obj.Voids.reserve(obj.VoidCount);
                    for (uint32_t vi = 0; vi < obj.VoidCount; vi++)
                    {
                        Cutout cutout;
                        cutout.Tag = r.ReadU32();
                        R_ASSERT(cutout.Tag <= 1);
                        cutout.EdgeCount = r.ReadU32();
                        cutout.Vertices.reserve(cutout.EdgeCount);
                        for (uint32_t ci = 0; ci < cutout.EdgeCount; ci++)
                            cutout.Vertices.push_back(r.ReadVec2S());
                        obj.Voids.push_back(std::move(cutout));
                    }
                    obj.VoidFlags = r.ReadU32();
                }
                Surfaces.push_back(std::move(obj));
            }
        }

        void LoadUnknownItems(StreamReader &r)
        {
            UnknownItemCount = r.ReadU32();
            UnknownItemsParam = r.ReadU32();
            if (UnknownItemCount)
            {
                UnknownItems.reserve(UnknownItemCount);
                for (uint32_t i = 0; i < UnknownItemCount; i++)
                    UnknownItems.emplace_back().Load(r);
                uint32_t skip = r.ReadU32();
                R_ASSERT(skip == 0);
            }
            uint32_t skip = r.ReadU32();
            R_ASSERT(skip == 7);
        }

        void LoadTestpoints(StreamReader &r)
        {
            TpCount = r.ReadU32();
            TestPoints.reserve(TpCount);
            for (uint32_t i = 0; i < TpCount; i++)
            {
                TestPoint obj;
                obj.Load(r);
                TestPoints.push_back(obj);
            }
            {
                uint32_t skip = r.ReadU32();
                R_ASSERT(skip == 0);
                skip = r.ReadU32();
                R_ASSERT(skip == 4);
            }
            TPS2Size = r.ReadU32();
            TPS2Param = r.ReadU32();
            TestPoints2.reserve(TPS2Size);
            for (uint32_t i = 0; i < TPS2Size; i++)
            {
                TestPoint2 obj;
                obj.Load(r);
                TestPoints2.push_back(obj);
            }
            TPS3Size = r.ReadU32();
            TPS3Param = r.ReadU32();
            TestPoints3.reserve(TPS3Size);
            for (uint32_t i = 0; i < TPS3Size; i++)
            {
                TestPoint2 obj;
                obj.Load(r);
                TestPoints3.push_back(obj);
            }
            TestSequenceSize = r.ReadU32();
            TestSequenceParam = r.ReadU32();
            TestSequence.reserve(TestSequenceSize);
            for (uint32_t i = 0; i < TestSequenceSize; i++)
            {
                TestNode obj;
                obj.Load(r);
                TestSequence.push_back(obj);
            }
            if (TestSequenceParam == 1)
            {
                uint32_t skip[3];
                r.Read(skip, 3);
                R_ASSERT(skip[0] == 0);
                R_ASSERT(skip[1] == 0);
                R_ASSERT(skip[2] == 0);
            }
        }

        virtual void Load(StreamReader &r) override
        {
            Object::Load(r);
            LoadShapes(r);
            if (!Shapes.empty())
            {
                bool extraData = false;
                {
                    auto skip1 = r.ReadU32();
                    // NOTE: this might be incorrect way to determine data order
                    // (maybe it has to be bound to layer type?)
                    switch (skip1)
                    {
                    default:
                        R_ASSERT(!"Unrecognized data order");
                        break;
                    case 1: // normal: shapes, [these flags], pads, lines, arcs, surfaces
                        break;
                    case 2: // extra data: shapes, [these flags], pads, lines(0), arcs(0), surfaces, int32[4], lines, arcs
                        extraData = true;
                        break;
                    }
                    auto skip2 = r.ReadU32();
                    R_ASSERT(skip2 == 0);
                    auto skip3 = r.ReadU32();
                    R_ASSERT(skip3 == 1);
                }
                LoadPads(r);
                LoadLines(r);
                LoadArcs(r);
                LoadSurfaces(r);
                if (extraData)
                {
                    uint32_t skip3[4];
                    r.Read(skip3, 4);
                    printf("* skip: %u, %u, %u, %u\n",
                        skip3[0], skip3[1], skip3[2], skip3[3]);
                    // reload lines and arcs
                    LoadLines(r);
                    LoadArcs(r);
                    uint32_t skip4 = r.ReadU32();
                    R_ASSERT(skip4 == 0);
                }
            }
            LoadUnknownItems(r);
            LoadTestpoints(r);
        }
    };

    struct ThroughLayer : public Object
    {
        struct Tool
        {
            bool Flag1, Flag2;
            Fixed32 Size;
            uint32_t Data5[5];
            uint8_t Data3[3]; // color?

            void Load(StreamReader &r)
            {
                Flag1 = r.ReadBool8();
                Flag2 = r.ReadBool8();
                Size = r.ReadU32();
                r.Read(Data5, 5);
                r.Read(Data3, 3);
            }
        };
        std::vector<Tool> Tools;
        struct DrillHole
        {
            //uint8_t Code; // = 0x08
            int32_t Net;
            uint32_t Tool;
            Vector2S Pos;

            void Load(StreamReader &r)
            {
                Net = r.ReadS32();
                Tool = r.ReadU32();
                Pos = r.ReadVec2S();
            }
        };
        std::vector<DrillHole> DrillHoles;
        struct DrillSlot
        {
            //uint8_t Code; // = 0x0A
            int32_t Net;
            uint32_t Tool;
            Vector2S Begin, End;
            uint32_t Zero; // = 0

            void Load(StreamReader &r)
            {
                Net = r.ReadS32();
                Tool = r.ReadU32();
                Begin = r.ReadVec2S();
                End = r.ReadVec2S();
                Zero = r.ReadU32();
            }
        };
        std::vector<DrillSlot> DrillSlots;

        ThroughLayer() : Object(ObjectType::Through)
        {}
    
        virtual void Load(StreamReader &r) override
        {
            Object::Load(r);
            auto zero = r.ReadU32();
            R_ASSERT(zero == 0);
            zero = r.ReadU32();
            R_ASSERT(zero == 0);
            auto toolCount = r.ReadU32();
            R_ASSERT(toolCount);
            toolCount--;
            Tools.reserve(toolCount);
            for (uint32_t i = 0; i < toolCount; i++)
                Tools.emplace_back().Load(r);
            zero = r.ReadU8();
            R_ASSERT(zero == 0);
            auto drillCount = r.ReadU32();
            {
                auto v2 = r.ReadU32();
                printf("- drill holes[%u], v2[%u]\n", drillCount, v2);
                // skip 4 zero dwords
                uint32_t dummy[4];
                r.Read(dummy, 4);
                R_ASSERT(dummy[0] == 0);
                R_ASSERT(dummy[1] == 0);
                R_ASSERT(dummy[2] == 0);
                R_ASSERT(dummy[3] == 0);
            }
            // 08 : drill hole
            // 0A : drill slot
            DrillHoles.reserve(drillCount);
            // XXX: drill slot count must be stored somewhere
            DrillSlots.reserve(100);
            for (uint32_t i = 0; i < drillCount; i++)
            {
                auto code = r.ReadU8();
                if (code == 0x08)
                {
                    DrillHoles.emplace_back().Load(r);
                    continue;
                }
                if (code == 0x0A)
                {
                    DrillSlots.emplace_back().Load(r);
                    continue;
                }
                R_ASSERT(!"Unrecognized drill code");
            }
        }
    };

    inline std::unique_ptr<Object> LoadObject(StreamReader &r)
    {
        Object *object = nullptr;
        ObjectType type = Object::Detect(r);
        switch (type)
        {
        case ObjectType::Logic:
            object = new LogicLayer();
            break;
        case ObjectType::Through:
            object = new ThroughLayer();        
            break;
        default:
            R_ASSERT(!"Unrecognized ObjectType");
            break;
        }
        if (object)
        {
            object->Load(r);
            printf("- done at addr[0x%08X]\n", uint32_t(r.Tell()));
        }
        return std::unique_ptr<Object>(object);
    }

    struct ProbeBox32
    {
        int32_t Tag;
        Vector2S V1, V2;

        void Load(StreamReader &r)
        {
            Tag = r.ReadS32();
            V1 = r.ReadVec2S();
            V2 = r.ReadVec2S();
        }
    };

    struct DoubleBox32
    {
        uint32_t Tag;
        ProbeBox32 B1, B2;

        void Load(StreamReader &r)
        {
            Tag = r.ReadU32();
            B1.Load(r);
            B2.Load(r);
        }
    };

    struct ProbeBox8
    {
        int8_t Tag;
        int32_t N, A, P1, P2;

        void Load(StreamReader &r)
        {
            Tag = r.ReadU8();
            N = r.ReadS32();
            A = r.ReadS32();
            P1 = r.ReadS32();
            P2 = r.ReadS32();
        }
    };

    struct ProbeDataItem
    {
        bool Present = false;
        Fixed32 Size = 0;
        uint32_t Params[5] = {};
        uint32_t Color = 0;

        void Load(StreamReader &r)
        {
            Present = r.ReadBool8();
            if (Present)
            {
                Size = r.ReadS32();
                r.Read(Params, 5);
                Color = r.ReadU32();
            }
        }
    };

    struct FixtureData
    {
        uint32_t P1;
        uint32_t PX[6];
        bool Flags[3];
        std::vector<ProbeDataItem> Items;
        // u32 ProbeBox8_count
        uint32_t C1;
        Vector2S V1, V2;
        std::vector<ProbeBox8> Boxes;

        void Load(StreamReader &r)
        {
            P1 = r.ReadU32();
            r.Read(PX, 6);
            r.Read(Flags, 3);
            uint32_t itemCount = r.ReadU32();
            R_ASSERT(itemCount > 0);
            Items.reserve(itemCount);
            for (uint32_t i = 0; i < itemCount; i++)
                Items.emplace_back().Load(r);
            uint32_t boxCount = r.ReadU32();
            C1 = r.ReadU32();
            V1 = r.ReadVec2S();
            V2 = r.ReadVec2S();
            Boxes.reserve(boxCount);
            for (uint32_t i = 0; i < boxCount; i++)
                Boxes.emplace_back().Load(r);
        }
    };

    struct ProbeData
    {
        FixtureData Fixture;
        Vector2S V3, V4;
        std::vector<DoubleBox32> Boxes2;

        void Load(StreamReader &r)
        {
            Fixture.Load(r);
            V3 = r.ReadVec2S();
            V4 = r.ReadVec2S();
            uint32_t box2Count = r.ReadU32();
            Boxes2.reserve(box2Count);
            for (uint32_t i = 0; i < box2Count; i++)
                Boxes2.emplace_back().Load(r);
        }
    };

    struct Probe
    {
        struct
        {
            bool Flag; // 01
            uint32_t Tag; // 15
            std::string Name; // "Spear_B_100 Mil"
            Fixed32 Size1; // 7000
            uint32_t Param1; // 0x0012CCFC
            Fixed32 Size2; // 2000
            uint32_t Param2; // 98
            Fixed32 Size3; // 3000
            uint32_t Param3; // 5299
            uint32_t Color; // 0x0000FFFF
            uint32_t K1, V1;
            uint32_t K2, V2;
            uint32_t K3, V3;
            uint32_t K4, V4;
        } Header;
        std::unique_ptr<ProbeData> Body;
        struct
        {
            uint32_t Tag;
            bool Flag1, Flag2, Flag3;
            uint8_t P0;
            int32_t P1, P2, P3;        
            ProbeBox32 B1, B2;
        } Tail;

        void Load(StreamReader &r)
        {
            Header.Flag = r.ReadBool8();
            Header.Tag = r.ReadU32();
            Header.Name = r.ReadString255();
            Header.Size1 = r.ReadS32();
            Header.Param1 = r.ReadU32();
            Header.Size2 = r.ReadS32();
            Header.Param2 = r.ReadU32();
            Header.Size3 = r.ReadS32();
            Header.Param3 = r.ReadU32();
            Header.Color = r.ReadU32();
            Header.K1 = r.ReadU32();
            Header.V1 = r.ReadU32();
            Header.K2 = r.ReadU32();
            Header.V2 = r.ReadU32();
            Header.K3 = r.ReadU32();
            Header.V3 = r.ReadU32();
            Header.K4 = r.ReadU32();
            Header.V4 = r.ReadU32();
            bool hasBody = r.ReadBool8();
            if (hasBody)
            {
                ProbeData body;
                body.Load(r);
                Body = std::make_unique<ProbeData>();
                *Body = std::move(body);
            }
            Tail.Tag = r.ReadU32();
            Tail.Flag1 = r.ReadBool8();
            Tail.Flag2 = r.ReadBool8();
            Tail.Flag3 = r.ReadBool8();
            Tail.P0 = r.ReadU8();
            Tail.P1 = r.ReadS32();
            Tail.P2 = r.ReadS32();
            Tail.P3 = r.ReadS32();
            Tail.B1.Tag = r.ReadS32();
            Tail.B1.V1 = r.ReadVec2S();
            Tail.B1.V2 = r.ReadVec2S();
            Tail.B2.Tag = r.ReadS32();
            Tail.B2.V1 = r.ReadVec2S();
            Tail.B2.V2 = r.ReadVec2S();
        }
    };

    using ProbePack = std::vector<Probe>;

    struct ProbeRegistry
    {
        uint32_t Z1, Z2; // 0, 0
        uint32_t Param; // 4
        std::string Name;
        Fixed32 DefaultSize; // 118.11 mil = 3 mm
        std::vector<ProbePack> Packs;

        void Load(StreamReader &r)
        {
            Z1 = r.ReadU32();
            R_ASSERT(Z1 == 0);
            Z2 = r.ReadU32();
            R_ASSERT(Z2 == 0);
            Param = r.ReadU32();
            R_ASSERT(Param == 4);
            Name = r.ReadString255();
            DefaultSize = r.ReadU32();
            uint32_t packCount = r.ReadU32();
            R_ASSERT(packCount > 0);
            Packs.reserve(packCount);
            for (uint32_t ip = 0; ip < packCount; ip++)
            {
                ProbePack pack;
                uint32_t probeCount = r.ReadU32();
                for (uint32_t i = 0; i < probeCount; i++)
                {
                    Probe p;
                    p.Load(r);
                    pack.push_back(std::move(p));
                }
                Packs.push_back(std::move(pack));
            }
        }
    };

    struct FixtureVariant
    {
        std::string Name;
        std::string ShortName;
        bool Flag1; // = 1
        bool Flag2; // = 0
        FixtureData Data;

        void Load(StreamReader &r)
        {
            Name = r.ReadString255();
            ShortName = r.ReadString255();
            Flag1 = r.ReadBool8();
            Flag2 = r.ReadBool8();
            Data.Load(r);
        }
    };

    struct FixtureSetting
    {
        uint32_t Tag;
        std::string Name;
        uint32_t Param;
        std::vector<FixtureVariant> Variants;
        Vector2S WorkspaceSize;

        void Load(StreamReader &r)
        {
            Tag = r.ReadU32();
            R_ASSERT(Tag == 3);
            Name = r.ReadString255();
            Param = r.ReadU32();
            R_ASSERT(Param == 0);
            uint32_t variantCount = r.ReadU32();
            Variants.reserve(variantCount);
            for (uint32_t i = 0; i < variantCount; i++)
                Variants.emplace_back().Load(r);
            WorkspaceSize = r.ReadVec2S();
        }
    };

    struct FixtureRegistry
    {
        uint32_t Tag1; // must be 0
        uint32_t Tag2; // must be 7874
        std::vector<std::string> Grids;
        FixtureSetting Top, Bottom;

        void Load(StreamReader &r)
        {
            Tag1 = r.ReadU32();
            R_ASSERT(Tag1 == 0);
            Tag2 = r.ReadU32();
            R_ASSERT(Tag2 == 7874);
            Grids.reserve(8);
            for (uint32_t i = 0; i < 8; i++)
                Grids.push_back(r.ReadString255());
            Top.Load(r);
            Bottom.Load(r);
        }
    };

    struct Pin
    {
        uint32_t Handle;
        uint32_t Z1;
        uint32_t Id;
        std::string Name;
        uint32_t Z2;

        void Load(StreamReader &r)
        {
            Handle = r.ReadU32();
            Z1 = r.ReadU32();
            R_ASSERT(Z1 == 0);
            Id = r.ReadU32();
            Name = r.ReadString255();
            Z2 = r.ReadU32();
            R_ASSERT(Z2 == 0);
        }
    };

    enum class PartType : uint32_t
    {
        Chip = 0,
        Diode = 1,
        Transistor = 2,
        Resistor = 3,
        Unknown4 = 4,
        Capacitor = 5, // some fiducials (CFxx) go there for some reason
        Unknown6 = 6,
        Unknown7 = 7,
        Unknown8 = 8,
        Jumper = 9,
        Unknown10 = 10,
        Unknown11 = 11,
        Unknown12 = 12,
        Fuse = 13,
        Choke = 14,
        Oscillator = 15,
        Switch = 16,
        Connector = 17,
        Testpoint = 18, // transformers go there as well
        Unknown19 = 19,
        Unknown20 = 20,
        Mechanical = 21,
        Fiducial = 29, // FDxx
    };

    struct Part
    {
        std::string Name;
        Box2S Bbox;
        Vector2S Pos;
        int32_t Angle;
        uint32_t Decal;
        PartType Type;
        uint32_t Z1;
        Fixed32 Height;
        bool Flag0;
        std::string Value;
        std::string ToleranceP;
        std::string ToleranceN;
        std::string Desc;
        std::string Serial;
        uint32_t Z2 = 0;
        // uint32_t PinCount
        uint32_t Layer; // 3 / 12
        uint32_t P2; // 0
        std::vector<Pin> Pins;

        void Load(StreamReader &r)
        {
            Name = r.ReadString255();
            Bbox.Min = r.ReadVec2S();
            Bbox.Max = r.ReadVec2S();
            Pos = r.ReadVec2S();
            Angle = r.ReadS32();
            Decal = r.ReadU32();
            Type = static_cast<PartType>(r.ReadU32());
            Z1 = r.ReadU32();
            R_ASSERT(Z1 == 0);
            Height = r.ReadS32();
            Flag0 = r.ReadBool8();
            Value = r.ReadString255();
            ToleranceP = r.ReadString255();
            ToleranceN = r.ReadString255();
            Desc = r.ReadString255();
            if (Flag0)
            {
                Serial = r.ReadString255();
                Z2 = r.ReadU32();
                R_ASSERT(Z2 == 0);
            }
            auto pinCount = r.ReadU32();
            Layer = r.ReadU32();
            P2 = r.ReadU32();
            R_ASSERT(P2 == 0);
            Pins.reserve(pinCount);
            for (uint32_t i = 0; i < pinCount; i++)
                Pins.emplace_back().Load(r);
        }
    };

    struct MysteriousBlock
    {
        uint32_t P1, P2; // 50, 12452
        Vector2S TopRight;
        uint32_t P3, P4; // 3, 1
        bool Flag1, Flag2;
        uint8_t P5, P6;
        uint32_t P7x[4];
        bool Flags[6];
        uint32_t P8; // 10000
        uint32_t P9, P10, P11;
        uint8_t P12, P13;

        void Load(StreamReader &r)
        {
            P1 = r.ReadU32();
            P2 = r.ReadU32();
            TopRight = r.ReadVec2S();
            P3 = r.ReadU32();
            P4 = r.ReadU32();
            Flag1 = r.ReadBool8();
            Flag2 = r.ReadBool8();
            P5 = r.ReadU8();
            P6 = r.ReadU8();
            r.Read(P7x, 4);
            r.Read(Flags, 6);
            P8 = r.ReadU32();
            P9 = r.ReadU32();
            P10 = r.ReadU32();
            P11 = r.ReadU32();
            P12 = r.ReadU8();
            P13 = r.ReadU8();
        }
    };

    struct Decal
    {
        bool Flag1;
        std::string Name;

        uint32_t HeaderParams[3];
        bool Flag;
        std::array<std::unique_ptr<Object>, 3> Layers;
    
        bool OutlineFlag;
        uint32_t Param; // 2
        int32_t N1; // -1
        uint32_t OutlineVertexCount; // 8
        std::vector<Vector2S> Outline;
        uint32_t Params[2]; // 0, 0

        void Load(StreamReader &r)
        {
            Flag1 = r.ReadBool8();
            R_ASSERT(Flag1);
            Name = r.ReadString255();
            r.Read(HeaderParams, 3);
            Flag = r.ReadBool8();
            for (uint32_t i = 0; i < Layers.size(); i++)
            {
                bool present = r.ReadBool8();
                if (!present)
                    continue;
                Layers[i] = LoadObject(r);
            }
            OutlineFlag = r.ReadBool8();
            R_ASSERT(OutlineFlag);
            Param = r.ReadU32();
            N1 = r.ReadS32();
            OutlineVertexCount = r.ReadU32();
            Outline.reserve(OutlineVertexCount);
            for (uint32_t i = 0; i < OutlineVertexCount; i++)
            {
                Vector2S v = r.ReadVec2S();
                Outline.push_back(v);
            }
            r.Read(Params, 2);
        }
    };

    struct Board
    {
        TvwHeader Header;
        std::vector<std::unique_ptr<Object>> Layers;
        std::vector<std::string> Nets;
        ProbeRegistry Probes;
        FixtureRegistry Fixtures;
        MysteriousBlock Myb;
        std::vector<Part> Parts;
        std::vector<Decal> Decals;

    private:
        void ReadNetList(StreamReader &r)
        {
            uint32_t netCount = r.ReadU32();
            uint32_t nc2 = r.ReadU32();
            R_ASSERT(netCount > 0 && netCount == nc2);
            printf("- loading %u nets\n", netCount);
            Nets.reserve(netCount);
            for (uint32_t i = 0; i < netCount; i++)
                Nets.push_back(r.ReadString255());
        }

        void ReadParts(StreamReader &r)
        {
            uint32_t partCount = r.ReadU32();
            uint32_t skip = r.ReadU32();
            printf("- loading %u parts\n", partCount);
            Parts.reserve(partCount);
            for (uint32_t i = 0; i < partCount; i++)
                Parts.emplace_back().Load(r);
        }

        void ReadDecals(StreamReader &r)
        {
            uint32_t c = r.ReadU32();
            R_ASSERT(c == 3);
            uint32_t decalCount = r.ReadU32();
            printf("- loading %u decals\n", decalCount);
            Decals.reserve(decalCount);
            for (uint32_t i = 0; i < decalCount; i++)
                Decals.emplace_back().Load(r);
        }

    public:
        void Load(std::istream &fs)
        {
            StreamReader r(fs);
            Header.Load(r);
            Layers.reserve(Header.LayerCount);
            for (uint32_t li = 0; li < Header.LayerCount; li++)
            {
                std::unique_ptr<Object> layer = LoadObject(r);
                Layers.push_back(std::move(layer));
            }
            { // skip 4 zero dwords
                uint32_t dummy[4];
                r.Read(dummy, 4);
                R_ASSERT(dummy[0] == 0);
                R_ASSERT(dummy[1] == 0);
                R_ASSERT(dummy[2] == 0);
                R_ASSERT(dummy[3] == 0);
            }
            ReadNetList(r);
            Probes.Load(r);
            Fixtures.Load(r);
            Myb.Load(r);
            ReadParts(r);
            ReadDecals(r);
            printf("- done reading at addr[0x%08X]\n", uint32_t(r.Tell()));
        }
    };
} // namespace Tebo
