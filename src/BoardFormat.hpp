// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include <iostream> // istream, ostream
#include <memory> // unique_ptr
#include <map>
#include <string>
#include <type_traits> // enable_if, is_base_of

namespace CBF
{
    class Board;
}

class BoardFormat;

class BoardFormatRep
{
public:
    using FactoryFunc = BoardFormat *(*)();

private:
    FactoryFunc factory;

protected:
    template <typename TBoardFormat, typename = std::enable_if_t<std::is_base_of_v<BoardFormat, TBoardFormat>>>
    BoardFormatRep(TBoardFormat *) :
        factory([]() -> BoardFormat * { return new TBoardFormat(); })
    {}

public:
    virtual ~BoardFormatRep() = default;
    virtual char const *Tag() const { return ""; }
    virtual char const *Desc() const { return ""; }
    virtual bool CanRead() const { return false; }
    virtual bool CanWrite() const { return false; }
    FactoryFunc Factory() const { return factory; }
};

class BoardFormat
{
public:
    virtual ~BoardFormat() = default;    
    virtual void Read(std::istream &) { R_ASSERT(!"Not supported"); }
    virtual void Export(CBF::Board &) const { R_ASSERT(!"Not supported"); }
    virtual void Import(CBF::Board const &) { R_ASSERT(!"Not supported"); }
    virtual void Write(std::ostream &) const { R_ASSERT(!"Not supported"); }
    virtual BoardFormatRep const &Frep() const = 0;
    
private:
    static inline std::map<std::string, BoardFormatRep const &> registry;

public:
    static void Register(BoardFormatRep const &frep);
    static std::unique_ptr<BoardFormat> Create(char const *tag);
};
