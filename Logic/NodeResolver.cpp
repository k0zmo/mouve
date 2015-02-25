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

#include "NodeResolver.h"
#include "NodeTree.h"
#include "NodeType.h"
#include "NodeException.h"
#include "Kommon/TypeTraits.h"

#include <regex>

NodeResolver::NodeResolver(std::shared_ptr<NodeTree> nodeTree)
    : _nodeTree(std::move(nodeTree))
{
    if (!_nodeTree) throw std::logic_error("nodeTree is null");
}

int NodeResolver::resolve(const std::string& uri)
{   
    std::regex rx("^(i|o|p)://([^/]+)/(.*)$");
    std::smatch match;

    if(std::regex_match(uri, match, rx))
    {
        if(match.size() == 3 + 1)
        {
            const std::string& proto = match[1].str();
            const std::string& nodeName = match[2].str();
            const std::string& socketOrPropertyName = match[3].str();

            if (proto == "i")
                return resolveInputSocket(nodeName, socketOrPropertyName);
            else if (proto == "o")
                return resolveOutputSocket(nodeName, socketOrPropertyName);
            else if (proto == "p")
                return resolveProperty(nodeName, socketOrPropertyName);
        }
    }

    return -1;
}

NodeID NodeResolver::resolveNode(const std::string& nodeName)
{
    return _nodeTree->resolveNode(nodeName);
}

namespace
{
    // Default values for some types
    template <class Value>
    struct DefaultValue;

    template <>
    struct DefaultValue<SocketID>
    {
        static const SocketID value = InvalidSocketID;
    };

    template <>
    struct DefaultValue<PropertyID>
    {
        static const PropertyID value = InvalidPropertyID;
    };

    template <>
    struct DefaultValue<const PropertyConfig&>
    {
        static const PropertyConfig value;
    };

    static NodeProperty invalidNodeProperty{};
    const PropertyConfig DefaultValue<const PropertyConfig&>::value = 
        PropertyConfig{InvalidPropertyID, std::string{}, invalidNodeProperty};

    template <>
    struct DefaultValue<const SocketConfig&>
    {
        static const SocketConfig value;
    };

    const SocketConfig DefaultValue<const SocketConfig&>::value = 
        SocketConfig{InvalidSocketID, std::string{}, ENodeFlowDataType::Invalid};

    template <class T>
    typename std::vector<T>::const_iterator 
        findConfigWithName(const std::vector<T>& container, 
            const std::string& name) 
    {
        return std::find_if(container.begin(), container.end(),
            [&name](const T& cfg) {
                return cfg.name() == name;
            });
    }

    // True for all Callable with the signature RetValue F(const NodeConfig&)
    template <class Func>
    struct is_node_config_func
        : public std::integral_constant<
              bool, func_traits<Func>::arity == 1 &&
                        std::is_same<
                            typename func_traits<Func>::template arg<0>::type,
                            const NodeConfig&>::value>
    {
    };
}

template <class Func>
auto NodeResolver::decorateConfiguration(NodeID nodeID, Func&& func)
    -> typename func_traits<Func>::return_type
{
    static_assert(is_node_config_func<typename std::remove_reference<Func>::type>::value,
                  "Func must have signature: RetValue F(const NodeConfig&)");
    try
    {
        const NodeConfig& nodeConfig = _nodeTree->nodeConfiguration(nodeID);
        return func(nodeConfig);
    }
    catch(BadNodeException&)
    {
        using ReturnType = typename func_traits<Func>::return_type;
        return DefaultValue<ReturnType>::value;
    }
}

SocketID NodeResolver::resolveInputSocket(NodeID nodeID, 
                                          const std::string& socketName)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> SocketID
        {
            auto iter = findConfigWithName(cfg.inputs(), socketName);
            if (iter != cfg.inputs().end())
                return iter->socketID();
            return DefaultValue<SocketID>::value;
        });
}

SocketID NodeResolver::resolveInputSocket(const std::string& nodeName, 
                                          const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveInputSocket(nodeID, socketName);
    return DefaultValue<SocketID>::value;
}

const SocketConfig& NodeResolver::inputSocketConfig(NodeID nodeID, 
                                                    SocketID socketID)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> const SocketConfig&
        {
            if(socketID < cfg.inputs().size())
                return cfg.inputs()[socketID];
            return DefaultValue<const SocketConfig&>::value;
        });
}

const SocketConfig& NodeResolver::inputSocketConfig(NodeID nodeID,
                                                    const std::string& socketName)
{
    return decorateConfiguration(nodeID,
        [&](const NodeConfig& cfg) -> const SocketConfig& 
        {
            auto iter = findConfigWithName(cfg.inputs(), socketName);
            if (iter != cfg.inputs().end())
                return *iter;
            return DefaultValue<const SocketConfig&>::value;
        });
}

const SocketConfig& NodeResolver::inputSocketConfig(const std::string& nodeName,
                                                    const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return inputSocketConfig(nodeID, socketName);
    return DefaultValue<const SocketConfig&>::value;
}

SocketID NodeResolver::resolveOutputSocket(NodeID nodeID,
                                           const std::string& socketName)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> SocketID
        {
            auto iter = findConfigWithName(cfg.outputs(), socketName);
            if (iter != cfg.outputs().end())
                return iter->socketID();
            return DefaultValue<SocketID>::value;
        });
}

SocketID NodeResolver::resolveOutputSocket(const std::string& nodeName,
                                           const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveOutputSocket(nodeID, socketName);
    return DefaultValue<SocketID>::value;
}

const SocketConfig& NodeResolver::outputSocketConfig(NodeID nodeID,
                                                     SocketID socketID)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> const SocketConfig&
        {
            if(socketID < cfg.outputs().size())
                return cfg.outputs()[socketID];
            return DefaultValue<const SocketConfig&>::value;
        });
}

const SocketConfig& NodeResolver::outputSocketConfig(NodeID nodeID,
                                                     const std::string& socketName)
{
    return decorateConfiguration(nodeID,
        [&](const NodeConfig& cfg) -> const SocketConfig& 
        {
            auto iter = findConfigWithName(cfg.outputs(), socketName);
            if (iter != cfg.outputs().end())
                return *iter;
            return DefaultValue<const SocketConfig&>::value;
        });
}

const SocketConfig& NodeResolver::outputSocketConfig(const std::string& nodeName, 
                                                     const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return outputSocketConfig(nodeID, socketName);
    return DefaultValue<const SocketConfig&>::value;
}

PropertyID NodeResolver::resolveProperty(NodeID nodeID, 
                                         const std::string& propertyName)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> PropertyID
        {
            auto iter = findConfigWithName(cfg.properties(), propertyName);
            if (iter != cfg.properties().end())
                return iter->propertyID();
            return DefaultValue<PropertyID>::value;
        });
}

PropertyID NodeResolver::resolveProperty(const std::string& nodeName,
                                         const std::string& propertyName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveProperty(nodeID, propertyName);
    return DefaultValue<PropertyID>::value;
}

const PropertyConfig& NodeResolver::propertyConfig(NodeID nodeID, 
                                                   PropertyID propertyID)
{
    return decorateConfiguration(nodeID, 
        [&](const NodeConfig& cfg) -> const PropertyConfig&
        {
            if(propertyID >= 0 && static_cast<size_t>(propertyID) < cfg.properties().size())
                return cfg.properties()[propertyID];
            return DefaultValue<const PropertyConfig&>::value;
        });
}

const PropertyConfig& NodeResolver::propertyConfig(NodeID nodeID, 
                                                   const std::string& propertyName)
{
    return decorateConfiguration(nodeID,
        [&](const NodeConfig& cfg) -> const PropertyConfig& 
        {
            auto iter = findConfigWithName(cfg.properties(), propertyName);
            if (iter != cfg.properties().end())
                return *iter;
            return DefaultValue<const PropertyConfig&>::value;
        });
}

const PropertyConfig& NodeResolver::propertyConfig(const std::string& nodeName, 
                                                   const std::string& propertyName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return propertyConfig(nodeID, propertyName);
    return DefaultValue<const PropertyConfig&>::value;
}

SocketAddress NodeResolver::resolveSocketAddress(const std::string& uri)
{
    std::regex rx("^(i|o)://([^/]+)/(.*)$");
    std::smatch match;

    if(std::regex_match(uri, match, rx))
    {
        if(match.size() == 3 + 1)
        {
            const std::string& proto = match[1].str();
            const std::string& nodeName = match[2].str();
            const std::string& socket = match[3].str();

            NodeID nodeID = resolveNode(nodeName);
            if (nodeID != InvalidNodeID)
            {
                if (proto == "i")
                    return {nodeID, resolveInputSocket(nodeID, socket), false};
                else if (proto == "o")
                    return {nodeID, resolveOutputSocket(nodeID, socket), true};
            }
        }
    }

    return {InvalidNodeID, InvalidSocketID, false};
}

std::string NodeResolver::socketAddressUri(const SocketAddress& socketAddr)
{
    if(!socketAddr.isValid()) return {};

    std::ostringstream strm;

    if(socketAddr.isOutput)
    {
        strm << "o://";
        strm << _nodeTree->nodeName(socketAddr.node);
        strm << "/";
        strm << outputSocketConfig(socketAddr.node, socketAddr.socket).name();
    }
    else
    {
        strm << "i://";
        strm << _nodeTree->nodeName(socketAddr.node);
        strm << "/";
        strm << inputSocketConfig(socketAddr.node, socketAddr.socket).name();
    }    

    return strm.str();
}
