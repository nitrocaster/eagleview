// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#include <cstdio> // std::puts
#include <fstream> // std::ofstream
#include "BoardFormat.hpp"
#include "BoardFormatRegistrator.hpp"
#include "CBF/Board.hpp"

static void PrintUsage()
{
    puts("usage:\n"
        "    eagleview <input format> <input path> <output format> <output path>\n"
        "\nsupported formats:");
    using RegNode = BoardFormatRegistrator::Node;
    for (RegNode const *n = RegNode::First; n; n = n->Next)
    {
        auto const &frep = n->Frep;
        std::string caps;
        if (frep.CanRead())
            caps += 'r';
        if (frep.CanWrite())
            caps += 'w';
        if (caps.empty())
            caps += "-";
        printf("    -%s [%s] %s\n", frep.Tag(), caps.data(), frep.Desc());
    }
}

int main(int argc, char const *argv[])
{
    BoardFormatRegistrator::Register();
    if (argc != 5)
    {
        PrintUsage();
        return 1;
    }
    char const *srcFormat = argv[1],
        *srcPath = argv[2],
        *dstFormat = argv[3],
        *dstPath = argv[4];
    // XXX: catch exceptions
    auto src = BoardFormat::Create(srcFormat+1);
    if (!src)
    {
        puts("! Unrecognized input format");
        return 1;
    }
    if (!src->Frep().CanRead())
    {
        puts("! The input format is not readable");
        return 1;
    }
    auto dst = BoardFormat::Create(dstFormat+1);
    if (!dst)
    {
        puts("! Unrecognized output format");
        return 1;
    }
    if (!dst->Frep().CanWrite())
    {
        puts("! The output format is not writeable");
        return 1;
    }
    CBF::Board brd;
    {
        auto fs = std::ifstream(srcPath, std::ios::binary);
        if (!fs)
        {
            printf("! Can't read '%s'\n", srcPath);
            return 1;
        }
        src->Read(fs);
        src->Export(brd);
    }
    {
        auto fs = std::ofstream(dstPath, std::ios::binary);
        if (!fs)
        {
            printf("! Can't write '%s'\n", dstPath);
            return 1;
        }
        dst->Import(brd);
        dst->Write(fs);
    }
    return 0;
}
