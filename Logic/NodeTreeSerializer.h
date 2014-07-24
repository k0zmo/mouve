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

class QJsonObject;
class QJsonArray;
class QVariant;

class LOGIC_EXPORT NodeTreeSerializer
{
public:
    NodeTreeSerializer(std::string rootDir = "")
        : _rootDirectory(std::move(rootDir))
    {}

    bool serializeJsonToFile(const std::shared_ptr<NodeTree>& nodeTree,
        const std::string& filePath);

    // Serializes given node tree to JSON object
    QJsonObject serializeJson(const std::shared_ptr<NodeTree>& nodeTree);

    bool deserializeJsonFromFile(std::shared_ptr<NodeTree>& nodeTree, 
        const std::string& filePath);

    // Deserializes given JSON object to nodeTree object
    // Returns true upon a success
    bool deserializeJson(std::shared_ptr<NodeTree>& nodeTree, 
        const QJsonObject& jsonTree, 
        std::map<NodeID, NodeID>* oldToNewNodeID = nullptr);

private:
    bool deserializeJsonNodes(std::shared_ptr<NodeTree>& nodeTree,
        const QJsonArray& jsonNodes,
        std::map<NodeID, NodeID>& mapping);
    bool deserializeJsonLinks(std::shared_ptr<NodeTree>& nodeTree,
        const QJsonArray& jsonLinks,
        const std::map<NodeID, NodeID>& mapping);

    QJsonObject serializeProperty(PropertyID propID,
        const PropertyConfig& propertyConfig,
        const NodeProperty& propValue);
    EPropertyType deserializePropertyType(const std::string&);
    NodeProperty deserializeProperty(EPropertyType propType, 
        const QVariant& qvalue);

private:
    std::string _rootDirectory;
};

