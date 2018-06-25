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

class MOUVE_EXPORT NodeSystem
{
public:
    NodeSystem();
    ~NodeSystem();

    NodeSystem(NodeSystem const&) = delete;
    NodeSystem& operator=(NodeSystem const&) = delete;

    void registerAutoTypes(AutoRegisterNodeFactory* head);
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

    size_t loadPlugin(const std::string& pluginPath);
    void loadPlugins();

    int numRegisteredNodeTypes() const;

private:
    // That is all that NodeSystem needs for handling registered node types
    struct NodeTypeInfo
    {
        NodeTypeInfo(std::string nodeTypeName, std::unique_ptr<NodeFactory> nodeFactory)
            : nodeTypeName{std::move(nodeTypeName)}, nodeFactory{std::move(nodeFactory)}
        {
        }

        std::string nodeTypeName;
        std::unique_ptr<NodeFactory> nodeFactory;
    };

    std::vector<NodeTypeInfo> _registeredNodeTypes;
    std::unordered_map<std::string, NodeTypeID> _typeNameToTypeID;
    std::unordered_map<std::string, std::shared_ptr<NodeModule>> _registeredModules;
    std::unordered_map<std::string, std::shared_ptr<NodePlugin>> _plugins;

private:
    class NodeTypeIteratorImpl;
};

