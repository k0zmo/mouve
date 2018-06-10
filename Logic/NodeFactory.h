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
#include "Kommon/Utils.h"

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
    virtual std::unique_ptr<NodeType> create()
    {
        return std::make_unique<Type>();
    }
};

template <class Type>
std::unique_ptr<DefaultNodeFactory<Type>> makeDefaultNodeFactory()
{
    return std::make_unique<DefaultNodeFactory<Type>>();
}

class AutoRegisterNodeBase : public SList<AutoRegisterNodeBase>,
                             public NodeFactory
{
public:
    AutoRegisterNodeBase(const std::string& typeName)
        : typeName(typeName)
    {
    }

    std::string typeName;
};

template <class Type>
class AutoRegisterNode : public AutoRegisterNodeBase
{
public:
    AutoRegisterNode(const std::string& typeName)
        : AutoRegisterNodeBase{typeName}
    {
    }

    virtual std::unique_ptr<NodeType> create()
    {
        return std::make_unique<Type>();
    }
};

// Statically registers node type
#define REGISTER_NODE(NodeTypeName, NodeClass) \
    AutoRegisterNode<NodeClass> __auto_registered_##NodeClass(NodeTypeName);
