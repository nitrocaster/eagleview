// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#include "ToptestBoardview.hpp"
#include "ToptestImporter.hpp"
#include "ToptestWriter.hpp"
#include <cstdio> // std::puts
#include <fstream> // std::ofstream

static void PrintUsage()
{
    puts("usage: eagleview <eagle brd file name> {-brd} <output file name>");
}

int main(int argc, char const *argv[])
{
    if (argc != 4)
    {
        PrintUsage();
        return 1;
    }
    auto srcFile = argv[1];
    auto targetFormat = argv[2];
    auto dstFile = argv[3];
    using namespace tinyxml2;
    XMLDocument src;
    if (src.LoadFile(srcFile) != XML_SUCCESS)
    {
        printf("! %s\r\n", src.ErrorStr());
        return 1;
    }
    // XXX: catch exceptions
    Toptest::Boardview brd;
    auto fs = std::ofstream(dstFile);
    Toptest::Importer importer(brd);
    importer.Import(src);
    Toptest::Writer writer(brd);
    writer.Write(fs);
    return 0;
}
