// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#include "BoardFormat.hpp"

void BoardFormat::Register(BoardFormatRep const &frep)
{
    auto const it = registry.find(frep.Tag());
    R_ASSERT(it == registry.end());
    registry.emplace(frep.Tag(), frep);
}

std::unique_ptr<BoardFormat> BoardFormat::Create(char const *tag)
{
    auto const it = registry.find(tag);
    if (it == registry.end())
        return nullptr;
    return std::unique_ptr<BoardFormat>(it->second.Factory()());
}
