// MIT License
// Copyright (c) 2020 Pavel Kovalenko

#include "BoardFormatRegistrator.hpp"
#include "BoardFormat.hpp"

BoardFormatRegistrator::Node::Node(BoardFormatRep const &frep) :
    Frep(frep)
{
    InsertAfter(nullptr, this);
}

BoardFormatRegistrator::Node::~Node()
{
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    if (First == this)
        First = Next;
    if (Last == this)
        Last = Prev;
}

void BoardFormatRegistrator::Node::Register()
{
    if (Done)
        return;
    BoardFormat::Register(Frep);
    Done = true;
}

void BoardFormatRegistrator::Node::InsertAfter(Node* ref, Node* node)
{
    if (!ref)
    {
        node->Prev = nullptr;
        node->Next = First;
        if (First)
            First->Prev = node;
        else
            Last = node;
        First = node;
    }
    else
    {
        node->Prev = ref;
        node->Next = ref->Next;
        if (ref == Last)
            Last = node;
        ref->Next = node;
    }
    Count++;
}

void BoardFormatRegistrator::Register()
{
    for (auto node = Node::First; node; node = node->Next)
        node->Register();
}

void BoardFormatRegistrator::Reset()
{
    for (auto node = Node::First; node; node = node->Next)
        node->Done = false;
}
