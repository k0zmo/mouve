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
#include "NodeFactory.h"

class LOGIC_EXPORT NodeSystem
{
    K_DISABLE_COPY(NodeSystem)
public:
    NodeSystem();
    ~NodeSystem();

    NodeTypeID registerNodeType(const std::string& nodeTypeName,
                                std::unique_ptr<NodeFactory> nodeFactory);
    std::unique_ptr<NodeType> createNode(NodeTypeID nodeTypeID) const;

    std::unique_ptr<NodeTree> createNodeTree();

    // Complementary method for NodeTypeID <---> NodeTypeName conversion
    const std::string& nodeTypeName(NodeTypeID nodeTypeID) const;
    std::string nodeDescription(NodeTypeID nodeTypeID) const;
    NodeTypeID nodeTypeID(const std::string& nodeTypeName) const;
    std::string defaultNodeName(NodeTypeID nodeTypeID) const;

    std::unique_ptr<NodeTypeIterator> createNodeTypeIterator() const;

    bool registerNodeModule(const std::shared_ptr<NodeModule>& module);
    const std::shared_ptr<NodeModule>& nodeModule(const std::string& name);

    size_t loadPlugin(const std::string& pluginName);

    int numRegisteredNodeTypes() const;

private:
    // That is all that NodeSystem needs for handling registered node types
    class NodeTypeInfo
    {
        K_DISABLE_COPY(NodeTypeInfo)
    public:
        NodeTypeInfo(const std::string& nodeTypeName,
                     std::unique_ptr<NodeFactory> nodeFactory)
            : nodeTypeName(nodeTypeName)
            , nodeFactory(std::move(nodeFactory))
            , automaticallyRegistered(false)
        {}

        ~NodeTypeInfo();

        std::string nodeTypeName;
        std::unique_ptr<NodeFactory> nodeFactory;
        bool automaticallyRegistered;

        // Mandatory when using unique_ptr
        NodeTypeInfo(NodeTypeInfo&& rhs);
        NodeTypeInfo& operator=(NodeTypeInfo&& rhs);
    };

    std::vector<NodeTypeInfo> _registeredNodeTypes;
    std::unordered_map<std::string, NodeTypeID> _typeNameToTypeID;
    std::unordered_map<std::string, std::shared_ptr<NodeModule>> _registeredModules;
    std::unordered_map<std::string, std::unique_ptr<NodePlugin>> _plugins;

private:
    class NodeTypeIteratorImpl;

private:
    // This makes it easy for nodes to be registered automatically
    void registerAutoTypes();
};

