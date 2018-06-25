/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include "Prerequisites.h"

// Abstract node factory
class NodeFactory
{
public:
    virtual ~NodeFactory() {}
    virtual std::unique_ptr<NodeType> create() = 0;
};

// Default, typed node factory.
// Used when static registering is not possible (i.e. plugins)
template <class Type>
class DefaultNodeFactory : public NodeFactory
{
public:
    std::unique_ptr<NodeType> create() override { return std::make_unique<Type>(); }
};

template <class Type>
std::unique_ptr<DefaultNodeFactory<Type>> makeDefaultNodeFactory()
{
    return std::make_unique<DefaultNodeFactory<Type>>();
}

namespace detail {

template <class T>
class SList
{
public:
    SList()
    {
        _next = _head;
        _head = this;
    }

    static T* head() { return static_cast<T*>(_head); }
    T* next() const { return static_cast<T*>(_next); }

private:
    static SList* _head;
    SList* _next;
};

template <class T>
SList<T>* SList<T>::_head = nullptr;
} // namespace detail

class AutoRegisterNodeFactory : public detail::SList<AutoRegisterNodeFactory>
{
public:
    explicit AutoRegisterNodeFactory(const char* typeName) : _typeName{typeName} {}

    const std::string& typeName() const { return _typeName; }
    virtual std::unique_ptr<NodeFactory> factory() = 0;

private:
    std::string _typeName;
};

template <typename Node>
class AutoRegisterNodeFactoryImpl : public AutoRegisterNodeFactory
{
public:
    explicit AutoRegisterNodeFactoryImpl(const char* typeName) : AutoRegisterNodeFactory{typeName}
    {
    }

private:
    std::unique_ptr<NodeFactory> factory() override { return makeDefaultNodeFactory<Node>(); }
};

// Statically registers node type
#define REGISTER_NODE(NodeTypeName, NodeClass)                                                     \
    AutoRegisterNodeFactoryImpl<NodeClass> __auto_registered_##NodeClass(NodeTypeName);
