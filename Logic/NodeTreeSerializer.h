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
#include "Kommon/json11.hpp"

class serializer_exception : public std::runtime_error
{
public:
    explicit serializer_exception(const std::string& message)
        : std::runtime_error(message)
    {
    }

    explicit serializer_exception(const char* message)
        : std::runtime_error(message)
    {
    }
};

class MOUVE_EXPORT NodeTreeSerializer
{
public:
    NodeTreeSerializer(std::string rootDir = "")
        : _rootDirectory(std::move(rootDir))
    {
    }

    // Serialization - may throw on failure
    void serializeToFile(const NodeTree& nodeTree, const std::string& filePath);
    json11::Json::object serialize(const NodeTree& nodeTree);

    // Deserialization - may throw on failure
    json11::Json deserializeFromFile(NodeTree& nodeTree, // output
                                     const std::string& filePath);
    void deserialize(NodeTree& nodeTree, // output
                     const json11::Json& json);

    const std::map<NodeID, NodeID>& idMappings() const { return _idMappings; }
    const std::vector<std::string>& warnings() const { return _warnings; }

private:
    json11::Json serializeIO(const std::vector<SocketConfig>& ios);
    json11::Json serializeProperties(const Node* node,
                                     const std::vector<PropertyConfig>& props);

    void deserializeNodes(NodeTree& nodeTree, const json11::Json& jsonNodes);
    void deserializeLinks(NodeTree& nodeTree, const json11::Json& jsonLinks);
    void deserializeProperties(NodeTree& nodeTree, // skip on bad properties
                               const json11::Json& jsonProps, NodeID nodeID,
                               const std::string& nodeName);

private:
    std::string _rootDirectory;
    std::map<NodeID, NodeID> _idMappings; // contains old (read from
                                          // JSON) node ID <-> new ID
                                          // (given by node system
                                          // upon deserializing)
    std::vector<std::string> _warnings;
};
