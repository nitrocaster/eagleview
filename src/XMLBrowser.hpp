// MIT License
// Copyright (c) 2019 Pavel Kovalenko

#pragma once

#include <tinyxml2/tinyxml2.h>
#include <stdexcept> // std::runtime_error
#include <utility> // std::move

namespace tinyxml2
{
    class XMLException : public std::runtime_error
    {
    public:
        XMLException(char const *msg) :
            std::runtime_error(msg)
        {}

        XMLException(std::string const &msg) :
            std::runtime_error(msg)
        {}

        XMLException(std::string &&msg) :
            std::runtime_error(std::move(msg))
        {}
    };

    class XMLBrowser final
    {
    public:
        class Proxy final
        {
        private:
            XMLElement *child;

        public:
            Proxy(XMLElement *element) noexcept
            { child = element; }

            Proxy& Next(char const *name = nullptr) noexcept(false)
            {
                if (!child)
                    throw XMLException("Attempt to navigate past the last element");
                child = child->NextSiblingElement(name);
                return *this;
            }

            Proxy operator()(char const *childName, bool verifyChild = true) noexcept(false)
            {
                if (!child)
                    throw XMLException("Parent element is null");
                XMLElement *element = child->FirstChildElement(childName);
                if (verifyChild && !element)
                    throw XMLException(std::string("Child node not found: ") + childName);
                return Proxy(element);
            }

            Proxy Begin(char const *childName) noexcept(false)
            { return (*this)(childName, false); }

            constexpr operator bool() const noexcept
            { return !!child; }

            bool HasAttribute(char const *key) const noexcept(false)
            {
                if (!child)
                    throw XMLException("Invalid XML element.");
                return !!child->FindAttribute(key);
            }

            char const *String(char const *key) const noexcept(false)
            {
                if (!child)
                    throw XMLException("Invalid XML element.");
                const char* value;
                if (child->QueryStringAttribute(key, &value) != XML_SUCCESS)
                    throw XMLException(std::string("Attribute not found: ") + key);
                return value;
            }

            int32_t Int32(char const *key) const noexcept(false)
            {
                if (!child)
                    throw XMLException("Invalid XML element.");
                int32_t value;
                if (child->QueryIntAttribute(key, &value) != XML_SUCCESS)
                    throw XMLException(std::string("Attribute not found: ") + key);
                return value;
            }

            int64_t Int64(char const *key) const noexcept(false)
            {
                if (!child)
                    throw XMLException("Invalid XML element.");
                int64_t value;
                if (child->QueryInt64Attribute(key, &value) != XML_SUCCESS)
                    throw XMLException(std::string("Attribute not found: ") + key);
                return value;
            }

            float Float(char const *key) const noexcept(false)
            {
                if (!child)
                    throw XMLException("Invalid XML element.");
                float value;
                if (child->QueryFloatAttribute(key, &value) != XML_SUCCESS)
                    throw XMLException(std::string("Attribute not found: ") + key);
                return value;
            }
        };
    private:
        XMLDocument *src;

    public:
        XMLBrowser(XMLDocument &doc) noexcept
        { src = &doc; }

        Proxy operator()(char const *path) noexcept(false)
        {
            XMLElement *element = src->FirstChildElement(path);
            if (!element)
                throw XMLException(std::string("Child node not found: ") + path);
            return Proxy(element);
        }
    };
} // namespace tinyxml2
