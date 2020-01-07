// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#include "ToptestBoardview.hpp"
#include "ToptestEagleImporter.hpp"
#include "ToptestTeboImporter.hpp"
#include "ToptestWriter.hpp"
#include <cstdio> // std::puts
#include <fstream> // std::ofstream

static void PrintUsage()
{
    puts("usage: eagleview <input format> <input path> <output format> <output path>\n"
        "supported input formats:\n"
        "[-eagle] Autodesk EAGLE board (*.BRD)\n"
        "[-tebo] Tebo-ICT view (*.TVW)\n"
        "supported output formats:\n"
        "[-toptest] Toptest board view (*.BRD)");
}

using ImporterFunction = bool(*)(std::string const &srcPath, Toptest::Boardview &brd);

static bool ImportEagle(std::string const &srcPath, Toptest::Boardview &brd)
{
    using namespace tinyxml2;
    XMLDocument src;
    if (src.LoadFile(srcPath.c_str()) != XML_SUCCESS)
    {
        printf("! %s\n", src.ErrorStr());
        return false;
    }
    Toptest::EagleImporter importer(brd);
    importer.Import(src);
    return true;
}

static bool ImportTebo(std::string const &srcPath, Toptest::Boardview &brd)
{
    auto fs = std::ifstream(srcPath, std::ios::binary);
    Tebo::Board src;
    src.Load(fs);
    Toptest::TeboImporter importer(brd);
    importer.Import(src);
    return true;
}

static std::map<std::string, ImporterFunction> Importers
{
    {"-eagle", ImportEagle},
    {"-tebo", ImportTebo}
};

int main(int argc, char const *argv[])
{
    if (argc != 5)
    {
        PrintUsage();
        return 1;
    }
    std::string srcFormat = argv[1],
        srcPath = argv[2],
        dstFormat = argv[3],
        dstPath = argv[4];
    if (dstFormat != "-toptest")
    {
        puts("! Unrecognized output format");
        return 1;
    }
    // XXX: catch exceptions
    Toptest::Boardview brd;
    auto importerIt = Importers.find(srcFormat);
    if (importerIt == Importers.end())
    {
        puts("! Unrecognized input format");
        return 1;
    }
    if (!importerIt->second(srcPath, brd))
        return 1;
    auto fs = std::ofstream(dstPath);
    if (!fs)
    {
        printf("! Can't write '%s'\n", dstPath.c_str());
        return 1;
    }
    Toptest::Writer writer(brd);
    writer.Write(fs);
    return 0;
}
