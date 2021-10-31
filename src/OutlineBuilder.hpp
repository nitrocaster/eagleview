// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "Vector2.hpp"
#include "Box2.hpp"
#include "Edge2.hpp"
#include <array>
#include <deque>
#include <vector>
#include <unordered_map>
#include <string>

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
        Vector2d V;
        Index Self;
        std::array<Index, 2> Neighbors{};

        VertexData() = default;

        VertexData(Vector2d v, Index self) :
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
        uint64_t operator()(Vector2d const &vec) const noexcept
        {
            return uint64_t(vec.X*73856093) ^ uint64_t(vec.Y*83492791);
        }
    };

    std::vector<VertexData> vertices;
    std::unordered_map<Vector2d, Index, Vector2Hasher> vertexSet;
    using Loop = std::deque<Index>;
    std::vector<Loop> loops;

    Index FindVertex(Vector2d v)
    {
        Index &index = vertexSet[v];
        if (!index.Valid())
        {
            index = vertices.size();
            VertexData const data(v, index);
            vertices.push_back(data);
        }
        return index;
    }

    static std::string VectorToString(Vector2d v)
    { return std::to_string(v.X) + ", " + std::to_string(v.Y); }

    Index NextVertex(VertexData const &current, Index prev)
    {
        auto const pred = [&](Index next) { return next.Valid() && next != prev; };
        auto const next = std::find_if(begin(current.Neighbors), end(current.Neighbors), pred);
        if (next == current.Neighbors.end())
            return Index::InvalidValue;
        else
            return *next;
    }

    Loop NextLoop(std::vector<bool> &visited, Index entry)
    {
        Loop loop;
        bool searchBack = false;
        Index index = entry, prev = index;
        while (true)
        {
            while (true)
            {
                if (!index.Valid())
                {
                    // if this is a dead end, search from the entry again,
                    // as we may have started from the middle of an open loop
                    if (searchBack)
                    {
                        // finish if already been there
                        searchBack = false;
                        break;
                    }
                    searchBack = loop.size() > 1;
                    break;
                }
                if (visited[index])
                {
                    // current loop is closed, or there are no other loops,
                    // so don't search back
                    searchBack = false;
                    break;
                }
                if (searchBack)
                    loop.push_front(index);
                else
                    loop.push_back(index);
                visited[index] = true;
                VertexData const &vd = vertices[index];
                if (vd.Self != index)
                    throw std::runtime_error("Index from VertexData must be equal to the computed index");
                index = NextVertex(vd, prev);
                prev = vd.Self;
            }
            if (searchBack)
            {
                index = NextVertex(vertices[entry], loop[1]);
                prev = entry;
                continue;
            }
            break;
        }
        return loop;
    }

    Box2d CalculateBBox(Loop const& loop) const
    {
        Box2d bbox(vertices[*loop.begin()].V, 0);
        for (Index i : loop)
            bbox.Merge(vertices[i].V);
        return bbox;
    }

public:
    void AddEdge(Edge2d edge)
    {
        Index const ia = FindVertex(edge.A);
        Index const ib = FindVertex(edge.B);
        if (ia == ib) // skip degenerate edges
            return;
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

    void Build(std::vector<Vector2i> &output)
    {
        if (vertices.empty())
            return;
        std::vector<bool> visited;
        visited.resize(vertices.size());
        std::fill(begin(visited), end(visited), false);
        for (auto it = visited.begin(); it != visited.end(); it = std::find(it, visited.end(), false))
            loops.push_back(NextLoop(visited, Index(it - visited.begin())));
        Loop const *maxLoop = &loops.front();
        Box2d maxLoopBBox(vertices.front().V, 0);
        for (Loop const &loop : loops)
        {
            Box2d const bb = CalculateBBox(loop);
            if (bb.Contains(maxLoopBBox))
            {
                maxLoopBBox = bb;
                maxLoop = &loop;                    
            }
        }
        for (Index i : *maxLoop)
            output.push_back(vertices[i].V);
    }
};
