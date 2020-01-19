// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include "ToptestBoardview.hpp"
#include <ostream> // std::ostream

namespace Toptest
{
    class Writer final
    {
    private:
        Boardview const &brd;
    public:
        Writer(Boardview const &brd) :
            brd(brd)
        {}
    private:
        Box2 CalculateOutlineBox() const
        {
            auto box = Box2::Empty;
            for (Vector2 const &v : brd.Outline())
                box.Merge(v);
            return box;
        }

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

        static int32_t Round(float f)
        { return static_cast<int32_t>(std::round(f)); }

        class StreamWriter final
        {
        protected:
            std::ostream &os;
            bool flipY = false;
            bool shift = false;
            float outlineHeight = 0;

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

            void OutlineHeight(float h) { outlineHeight = h; }

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

            void Write(float v)
            { Write(Round(v)); }

            void Write(BoardLayer layer)
            { Write(EncodeLayer(layer)); }

            template <typename... Args>
            void Write(Args const &...args)
            { (Write(args), ...); }

            void Write(Vector2 v)
            {
                if (shift)
                    v.Y -= outlineHeight;
                Write(v.X, ' ', flipY ? -v.Y : v.Y);
            }

            void Write(Box2 b)
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

    public:
        void Write(std::ostream &s) const
        {
            auto const rn = '\n';
            Box2 outlineBox = CalculateOutlineBox();
            Vector2 outlineSize = outlineBox.Size();
            StreamWriter w(s);
            w.OutlineHeight(outlineSize.Y);
            // brdout: n_verts bbox_size
            // vertex1
            // vertex2
            // ...
            auto const &outline = brd.Outline();
            int64_t magic = 163LL*(Round(outline[0].X) + Round(outline[0].Y));
            magic += 80LL*(outline.size()+1);
            magic += 79LL*Round(outlineBox.Height());
            magic += 84LL*Round(outlineBox.Width());
            w.Write(magic, rn);
            w.Write("BRDOUT: ", outline.size() + 1, " ", outlineSize, rn);
            for (size_t i = 0; i < outline.size() + 1; i++)
                w.Write(outline[i % outline.size()], rn);
            w.Write(rn);
            // nets: size
            // index net_name
            w.Write("NETS: ", brd.Nets().size(), rn);
            for (size_t i = 0; i < brd.Nets().size(); i++)
                w.Write(i+1, ' ', brd.Nets()[i], rn);
            w.Write(rn);
            // parts: size
            // name bbox.min.x bbox.min.y bbox.max.x bbox.max.y first_pin layer
            w.Write("PARTS: ", brd.Parts().size(), rn);
            for (auto const &part : brd.Parts())
                w.Write(part.get(), rn);
            w.Write(rn);
            // pins: size
            // pos.x pos.y net_index layer
            w.Write("PINS: ", brd.Pins().size(), rn);
            for (auto const &pin : brd.Pins())
                w.Write(pin.get(), rn);
            w.Write(rn);
            // nails: size
            // pos.x pos.y net_index layer
            w.Write("NAILS: ", brd.TestPoints().size(), rn);
            for (auto const &nail : brd.TestPoints())
                w.Write(nail.get(), rn);        
        }
    };
} // namespace Toptest
