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
        "    eagleview <input format> <input path> <output format> <output path>\n");
    // XXX: print supported formats
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
    if (!src->Frep().CanImport())
    {
        puts("! Import is not supported for the input format");
        return 1;
    }
    auto dst = BoardFormat::Create(dstFormat+1);
    if (!dst)
    {
        puts("! Unrecognized output format");
        return 1;
    }
    if (!dst->Frep().CanExport())
    {
        puts("! Export is not supported for the output format");
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
        src->Import(brd, fs);
    }
    {
        auto fs = std::ofstream(dstPath, std::ios::binary);
        if (!fs)
        {
            printf("! Can't write '%s'\n", dstPath);
            return 1;
        }
        dst->Export(brd, fs);
    }
    return 0;
}
