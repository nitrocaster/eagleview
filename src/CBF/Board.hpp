// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "BoardFormat.hpp"
#include "DynamicConvertible.hpp"
#include "Vector2.hpp"
#include "Box2.hpp"
#include "Edge2.hpp"
#include "Angle.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace CBF
{
    // All sizes are in mils.
    using Scalar = double;
    using Box2 = Box2T<Scalar>;
    using Vector2 = Vector2T<Scalar>;
    using Edge2 = Edge2T<Scalar>;
    using Color = uint32_t;
    
    enum class LayerType : uint32_t
    {
        Document,
        Multilayer,
        Top,
        Bottom,
        Signal,
        Plane,
        SolderTop,
        SolderBottom,
        SilkTop,
        SilkBottom,
        PasteTop,
        PasteBottom,
        Drill,
        Route
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
        RoundRect = 2,
        Oblong = 3,
        Poly = 4,
        Octagon = 5,
    };

    class Round;
    class Rect;
    class RoundRect;
    class Oblong;
    class Poly;
    class Octagon;

    class Shape : public DynamicConvertible<Round, Rect, RoundRect, Oblong, Poly, Octagon>
    {
    public:
        ShapeType Type;
        Vector2 Size;
        std::string Name;

        Shape(ShapeType type, Vector2 size, std::string const &name) :
            Type(type),
            Size(size),
            Name(name)
        {}

        Shape(ShapeType type, Vector2 size) :
            Type(type),
            Size(size)
        {}

        virtual Box2 BBox() const
        { return Box2(Size); }

        virtual ~Shape() = 0;
    };

    inline Shape::~Shape() = default;

    class Round : public Shape
    {
    public:
        Round(Scalar width) :
            Shape(ShapeType::Round, Vector2(width, width))
        {}
    };

    class Rect : public Shape
    {
    public:
        Rect(Vector2 size) : Shape(ShapeType::Rect, size)
        {}

    protected:
        Rect(ShapeType type, Vector2 size) : Shape(type, size)
        {}
    };

    class RoundRect : public Rect
    {
    public:
        Scalar Radius;

        RoundRect(Vector2 size, Scalar radius) :
            Rect(ShapeType::RoundRect, size),
            Radius(radius)
        {}
    };

    class Oblong : public Shape
    {
    public:
        Oblong(Vector2 size) : Shape(ShapeType::Oblong, size)
        {}
    };

    class Poly : public Shape
    {
    public:
        struct Line : public Edge2
        {
            Scalar Width;
        };

        std::vector<Line> Lines;
        std::vector<Vector2> Vertices;

    private:
        Box2 bbox;

    public:
        Poly(Box2 bbox, std::string const &shapeName) :
            Shape(ShapeType::Poly, bbox.Size(), shapeName),
            bbox(bbox)
        {}

        virtual Box2 BBox() const override
        { return bbox; }
    };

    class Octagon : public Rect
    {
    public:
        Scalar Radius;

        Octagon(Vector2 size, Scalar radius) :
            Rect(ShapeType::Octagon, size),
            Radius(radius)
        {}
    };

    class Pad
    {
    public:
        uint32_t Net = uint32_t(~0);
        // XXX: support dual shapes (top/bottom)
        uint32_t Shape;
        Vector2 Pos; // global pos
        Angle Turn; // local turn (final turn is Pad.Turn + Part.Turn)
        // XXX: support custom holes?
        Vector2 HoleOffset; // local offset
        Vector2 HoleSize;
    };

    class Line;
    class Arc;
    class Surface;

    class Primitive : public DynamicConvertible<Line, Arc, Surface>
    {
    public:
        uint32_t Net;
        PrimitiveType Type;
        Scalar LineWidth;

        Primitive(PrimitiveType type, uint32_t net = -1, Scalar lineWidth = 0)
        {
            Net = net;
            Type = type;
            LineWidth = lineWidth;
        }

        virtual ~Primitive() = 0;
    };

    inline Primitive::~Primitive() = default;
    
    class Line : public Primitive, public Edge2
    {
    public:
        Line() : Primitive(PrimitiveType::Line)
        {}
    };

    class Arc : public Primitive
    {
    public:
        Vector2 Pos;
        Scalar Radius;
        Scalar StartAngle, SweepAngle;

        Arc() : Primitive(PrimitiveType::Arc)
        {}
    };

    class Cutout
    {
    public:
        std::vector<Vector2> Vertices;
    };

    class Surface : public Primitive
    {
    public:
        std::vector<Vector2> Vertices;
        std::vector<Cutout> Voids;

        Surface() : Primitive(PrimitiveType::Surface)
        {}
    };

    class TestPoint
    {
    public:
        Vector2 Pos;
        uint32_t Net;
    };
    
    class Hole
    {
    public:
        uint32_t Net;
        Scalar Width;
        Vector2 Pos;
    };
    
    class Slot : public Edge2
    {
    public:
        uint32_t Net;
        Scalar Width;
    };

    class ArcSlot
    {
    public:
        uint32_t Net;
        Vector2 Pos;
        Scalar Radius;
        Scalar StartAngle, SweepAngle;
    };

    class Pin
    {
    public:
        // Layer index - can point to top or bottom
        uint32_t Layer;
        // Pad index in the corresponding layer
        uint32_t Pad;
        // 1 + index of this pin in Part::Pins
        uint32_t Id;
        // Name from the datasheet, like "C6"
        std::string Name;
    };
    
    class Part
    {
    public:
        // Reference designator
        std::string Name;
        // Bounding box that includes pads and package
        Box2 Bbox;
        Vector2 Pos;
        Angle Turn;
        // Decal index
        uint32_t Decal;
        Scalar Height;
        std::string Value;
        std::string ToleranceP;
        std::string ToleranceN;
        // Usually a part number
        std::string Desc;
        // Layer index : must be either top or bottom (multilayer and embedded parts are not supported)
        uint32_t Layer;
        std::vector<Pin> Pins;
    };
    // Can be an outline or a courtyard
    class Decal
    {
    public:
        std::string Name;
        std::vector<Vector2> Outline; // Must not be empty
    };

    enum class LayerClass : uint32_t
    {
        Undefined = 0,
        Drill = 1,
        Logic = 3
    };

    class LogicLayer;
    class DrillLayer;

    class Layer : public DynamicConvertible<LogicLayer, DrillLayer>
    {
    public:
        const LayerClass Class;
        std::string Name;
        LayerType Type = LayerType::Document;
        Color PadColor = 0;
        Color LineColor = 0;

        Layer(LayerClass lc) : Class(lc)
        {
            R_ASSERT(lc==LayerClass::Logic || lc==LayerClass::Drill);
        }

        virtual ~Layer() = default;
    };

    class LogicLayer : public Layer
    {
    public:
        std::vector<std::unique_ptr<Shape>> Shapes;
        std::vector<Pad> Pads;
        std::vector<Line> Lines;
        std::vector<Arc> Arcs;
        std::vector<Poly> Polys;
        std::vector<TestPoint> TestPoints;

        LogicLayer() : Layer(LayerClass::Logic)
        {}
    };

    struct Range
    {
        uint32_t From, To;
    };

    class DrillLayer : public Layer
    {
    public:
        std::vector<Hole> Holes;
        std::vector<Slot> Slots;
        Range Span{0, 0};

        DrillLayer() : Layer(LayerClass::Drill)
        {}
    };
    
    class Board
    {
    public:
        std::vector<std::unique_ptr<Layer>> Layers;
        std::vector<std::string> Nets;
        std::vector<Part> Parts;
        std::vector<Decal> Decals;
    };
} // namespace CBF
