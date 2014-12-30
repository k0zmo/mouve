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

#include "NodeTreeSerializer.h"
#include "NodeTree.h"
#include "NodeType.h"
#include "NodeSystem.h"

#include "Kommon/StringUtils.h"
#include "Kommon/Utils.h"

#include <fstream>

#include <QString>
#include <QFileInfo>
#include <QDir>

namespace priv {

static std::string relativePath(const std::string& path, // base path
                                const std::string& relative)
{
    QDir rootDir{QString::fromStdString(path)};
    if(rootDir.exists())
        return rootDir.absoluteFilePath(QString::fromStdString(relative))
            .toStdString();
    return {};
}

static std::string absolutePath(const std::string& path)
{
    return QFileInfo{path.c_str()}.absolutePath().toStdString();
}

static std::string makeRelative(const std::string& basePath,
                                const std::string& path)
{
    QDir rootDir{QString::fromStdString(basePath)};

    if (rootDir.exists())
        return rootDir.relativeFilePath(QString::fromStdString(path))
            .toStdString();
    else
        return path;
}

static json11::Json serializeProperty(const PropertyConfig& propertyConfig,
                                      const NodeProperty& propValue,
                                      const std::string& rootDirectory)
{
    json11::Json::object json{{"id", propertyConfig.propertyID()},
                              {"name", propertyConfig.name()}};

    switch (propertyConfig.type())
    {
    case EPropertyType::Boolean:
        json.insert(std::make_pair("value", propValue.toBool()));
        json.insert(std::make_pair("type", "boolean"));
        break;
    case EPropertyType::Integer:
        json.insert(std::make_pair("value", propValue.toInt()));
        json.insert(std::make_pair("type", "integer"));
        break;
    case EPropertyType::Double:
        json.insert(std::make_pair("value", propValue.toDouble()));
        json.insert(std::make_pair("type", "double"));
        break;
    case EPropertyType::Enum:
        json.insert(std::make_pair("value", (int)propValue.toEnum().data()));
        json.insert(std::make_pair("type", "enum"));
        break;
    case EPropertyType::Matrix:
    {
        json11::Json::array jsonMatrix{};
        Matrix3x3 matrix = propValue.toMatrix3x3();
        for (double v : matrix.v)
            jsonMatrix.push_back(v);
        json.insert(std::make_pair("value", jsonMatrix));
        json.insert(std::make_pair("type", "matrix3x3"));
        break;
    }
    case EPropertyType::Filepath:
        json.insert(std::make_pair(
            "value",
            priv::makeRelative(rootDirectory, propValue.toFilepath().data())));
        json.insert(std::make_pair("type", "filepath"));
        break;
    case EPropertyType::String:
        json.insert(std::make_pair("value", propValue.toString()));
        json.insert(std::make_pair("type", "string"));
        break;
    case EPropertyType::Unknown:
        break;
    }

    return json;
}

static EPropertyType convertToPropertyType(const std::string& type)
{
    if (type == "boolean")
        return EPropertyType::Boolean;
    else if (type == "integer")
        return EPropertyType::Integer;
    else if (type == "double")
        return EPropertyType::Double;
    else if (type == "enum")
        return EPropertyType::Enum;
    else if (type == "matrix3x3")
        return EPropertyType::Matrix;
    else if (type == "filepath")
        return EPropertyType::Filepath;
    else if (type == "string")
        return EPropertyType::String;
    else
        return EPropertyType::Unknown;
}

static NodeProperty deserializeProperty(EPropertyType propType,
                                        const json11::Json& json,
                                        const std::string& rootDirectory)
{
    switch (propType)
    {
    case EPropertyType::Boolean:
        return NodeProperty{json.bool_value()};
    case EPropertyType::Integer:
        return NodeProperty{json.int_value()};
    case EPropertyType::Double:
        return NodeProperty{json.number_value()};
    case EPropertyType::Enum:
        return NodeProperty{Enum(json.int_value())};
    case EPropertyType::Matrix:
    {
        auto const& matrixList = json.array_items();
        Matrix3x3 matrix;
        for (size_t i = 0; i < matrixList.size() && i < 9; ++i)
            matrix.v[i] = matrixList[i].number_value();
        return NodeProperty{matrix};
    }
    case EPropertyType::Filepath:
        return NodeProperty{
            Filepath{relativePath(rootDirectory, json.string_value())}};
    case EPropertyType::String:
        return NodeProperty{json.string_value()};
    }

    return NodeProperty{};
}
}

void NodeTreeSerializer::serializeToFile(const NodeTree& nodeTree,
                                         const std::string& filePath)
{
    try
    {
        std::ofstream file{filePath, std::ios::out};
        file.exceptions(~std::ios::goodbit);
        file << json11::Json{serialize(nodeTree)}.pretty_print();
    }
    catch (std::exception&)
    {
        std::throw_with_nested(serializer_exception{
            string_format("Couldn't open target file: %s", filePath.c_str())});
    }
}

json11::Json::object NodeTreeSerializer::serialize(const NodeTree& nodeTree)
{
    // Iterate over all nodes and serialize it as JSON value of JSON array
    json11::Json::array jsonNodes;
    auto nodeIt = nodeTree.createNodeIterator();
    NodeID nodeID;
    while (const Node* node = nodeIt->next(nodeID))
    {
        // Must fields
        json11::Json::object jsonNode{{"id", nodeID},
                                      {"class", nodeTree.nodeTypeName(nodeID)},
                                      {"name", node->nodeName()}};

        // Optional fields (i/o, properties)
        auto const& nodeConfig = node->config();

        jsonNode.insert(std::make_pair("inputs", serializeIO(nodeConfig.inputs())));
        jsonNode.insert(std::make_pair("outputs", serializeIO(nodeConfig.outputs())));
        jsonNode.insert(std::make_pair(
            "properties", serializeProperties(node, nodeConfig.properties())));

        jsonNodes.push_back(std::move(jsonNode));
    }

    // Iterate over all links and serialize it as JSON value of JSON array
    json11::Json::array jsonLinks;
    auto linkIt = nodeTree.createNodeLinkIterator();
    NodeLink nodeLink;
    while(linkIt->next(nodeLink))
    {
        jsonLinks.push_back(
            json11::Json::object{{"fromNode", nodeLink.fromNode},
                                 {"fromSocket", nodeLink.fromSocket},
                                 {"toNode", nodeLink.toNode},
                                 {"toSocket", nodeLink.toSocket}});
    }

    return json11::Json::object{{"nodes", jsonNodes}, {"links", jsonLinks}};
}

json11::Json NodeTreeSerializer::serializeIO(const std::vector<SocketConfig>& ios)
{
    json11::Json::array jsonIOs;
    for (const auto& io : ios)
    {
        jsonIOs.push_back(
            json11::Json::object{{"id", io.socketID()},
                                 {"name", io.name()},
                                 {"type", std::to_string(io.type())}});
    }
    return jsonIOs;
}

json11::Json NodeTreeSerializer::serializeProperties(
    const Node* node, const std::vector<PropertyConfig>& props)
{
    json11::Json::array jsonProps;
    for (const auto& prop : props)
    {
        NodeProperty propValue = node->property(prop.propertyID());
        if (propValue.isValid())
            jsonProps.push_back(
                priv::serializeProperty(prop, propValue, _rootDirectory));
    }
    return jsonProps;
}

json11::Json
    NodeTreeSerializer::deserializeFromFile(NodeTree& nodeTree,
                                            const std::string& filePath)
{
    std::string contents;

    try
    {
        std::ifstream file{filePath, std::ios::in};
        file.exceptions(~std::ios::goodbit);

        contents = std::string(
            (std::istreambuf_iterator<char>(file)), // most vexing parse
             std::istreambuf_iterator<char>());
    }
    catch (std::exception&)
    {
        std::throw_with_nested(serializer_exception{
            string_format("Error during loading file: %s", filePath.c_str())});
    }

    std::string err;
    json11::Json json{json11::Json::parse(contents, err)};
    if (!err.empty())
    {
        throw serializer_exception(
            string_format("Error during parsing JSON file: %s\n"
                          "Error details: %s",
                          filePath.c_str(), err.c_str()));
    }

    if (_rootDirectory.empty())
        _rootDirectory = priv::absolutePath(filePath);
    deserialize(nodeTree, json);
    return json;
}

void NodeTreeSerializer::deserialize(NodeTree& nodeTree,
                                     const json11::Json& json)
{
    std::string err;
    auto cleanUp = [&] {
        nodeTree.clear();
        _idMappings.clear();
        _warnings.clear();
    };
    cleanUp();

    // Check basic schema
    if (!json.is_object() || !json.has_shape({{"links", json11::Json::ARRAY},
                                              {"nodes", json11::Json::ARRAY}},
                                             err))
    {
        throw serializer_exception{
            "Couldn't open node tree - JSON is ill-formed."};
    }

    try
    {
        deserializeNodes(nodeTree, json["nodes"]);
        deserializeLinks(nodeTree, json["links"]);
    }
    catch (std::exception&)
    {
        cleanUp();
        throw;
    }
}

void NodeTreeSerializer::deserializeNodes(NodeTree& nodeTree,
                                          const json11::Json& jsonNodes)
{
    std::string err;
    for (const auto& node : jsonNodes.array_items()) 
    {
        // check schema
        if (!node.has_shape({{"class", json11::Json::STRING},
                             {"name", json11::Json::STRING},
                             {"id", json11::Json::NUMBER}},
                            err)) 
        {
            throw serializer_exception{
                string_format("node fields are invalid: %s", err.c_str())};
        }

        NodeID nodeID = node["id"].int_value();
        std::string const& nodeTypeName = node["class"].string_value();
        std::string const& nodeName = node["name"].string_value();

        // Try to create new node
        NodeTypeID nodeTypeID = nodeTree.nodeSystem()->nodeTypeID(nodeTypeName);
        NodeID _nodeID = nodeTree.createNode(nodeTypeID, nodeName);
        if (_nodeID == InvalidNodeID)
        {
            throw serializer_exception{string_format(
                "Couldn't create node of id: %d and type name: %s", nodeID,
                nodeTypeName.c_str())};
        }

        _idMappings.insert(std::make_pair(nodeID, _nodeID));

        // IOs in json are there for pure informational reasons
        deserializeProperties(nodeTree, node["properties"], nodeID, nodeName);
    }
}

void NodeTreeSerializer::deserializeLinks(NodeTree& nodeTree,
                                          const json11::Json& jsonLinks)
{
    std::string err;
    for (const auto& link : jsonLinks.array_items())
    {
        // check schema
        if (!link.has_shape({{"fromNode", json11::Json::NUMBER},
                             {"fromSocket", json11::Json::NUMBER},
                             {"toNode", json11::Json::NUMBER},
                             {"toSocket", json11::Json::NUMBER}},
                            err))
        {
            throw serializer_exception{
                string_format("link fields are invalid: %s", err.c_str())};
        }

        NodeID fromNode = link["fromNode"].int_value();
        SocketID fromSocket = link["fromSocket"].int_value();
        NodeID toNode = link["toNode"].int_value();
        SocketID toSocket = link["toSocket"].int_value();

        NodeID fromNodeRefined = get_or_default(_idMappings, fromNode, InvalidNodeID);
        NodeID toNodeRefined = get_or_default(_idMappings, toNode, InvalidNodeID);

        if (fromNodeRefined == InvalidNodeID ||
            toNodeRefined == InvalidNodeID)
        {
            throw serializer_exception{string_format(
                "No such node to link with: %d",
                fromNodeRefined == InvalidNodeID ? fromNode : toNode)};
        }

        if (nodeTree.linkNodes(SocketAddress(fromNodeRefined, fromSocket, true),
                               SocketAddress(toNodeRefined, toSocket, false)) !=
            ELinkNodesResult::Ok)
        {
            throw serializer_exception{
                string_format("Couldn't link nodes %d:%d with %d:%d", fromNode,
                              fromSocket, toNode, toSocket)};
        }
    }
}

void NodeTreeSerializer::deserializeProperties(NodeTree& nodeTree,
    const json11::Json& jsonProps, NodeID nodeID, const std::string& nodeName)
{
    std::string err;
    for (const auto& prop : jsonProps.array_items())
    {
        // check schema
        if (!prop.has_shape({{"id", json11::Json::NUMBER},
                             {"type", json11::Json::STRING}},
                            err))
        {
            throw serializer_exception{
                string_format("property fields are invalid (id:%d, name:%s)",
                              nodeID, nodeName.c_str())};
        }

        int propID = prop["id"].int_value();
        std::string const& propTypeStr = prop["type"].string_value();
        EPropertyType propType = priv::convertToPropertyType(propTypeStr);

        if (propType == EPropertyType::Unknown)
        {
            throw serializer_exception{
                string_format("\"type\" is bad for property of id %d "
                              "(id:%d, name:%s)",
                              propID, nodeID, nodeName.c_str())};
        }

        NodeProperty nodeProperty =
            priv::deserializeProperty(propType, prop["value"], _rootDirectory);

        // Non-fatal exception (contracts could've changed)
        if (!nodeTree.nodeSetProperty(_idMappings[nodeID], propID,
                                      nodeProperty))
        {
            _warnings.push_back(
                string_format("Couldn't set loaded property %d (type: %s)",
                              propID, propTypeStr.c_str()));
        }
    }
}