// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#pragma once

#include "Common.hpp"
#include "BoardFormat.hpp"

class BoardFormatRegistrator
{
public:
    struct Node
    {
    public:
        BoardFormatRep const &Frep;
        bool Done = false;
        Node *Prev, *Next;
        static Node *First, *Last;
        static size_t Count;

        Node(BoardFormatRep const &frep);
        ~Node();

        void Register();

        static void InsertAfter(Node *target, Node *node);
    };

    BoardFormatRegistrator() = delete;
    static void Register();
    static void Reset();
};

#define REGISTER_FORMAT(frepConstRef) \
    static BoardFormatRegistrator::Node const RegNode(frepConstRef)
