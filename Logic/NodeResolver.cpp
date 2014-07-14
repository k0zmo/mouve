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
#include "Kommon/TypeTraits.h"

#include <regex>

namespace
{
    const PropertyConfig& invalidPropertyConfig();
    const InputSocketConfig& invalidInputSocketConfig();
    const OutputSocketConfig& invalidOutputSocketConfig();

    template <class RetValue, class Pred>
    RetValue find(NodeTree& nodeTree, NodeID nodeID, Pred pred);
}

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

SocketID NodeResolver::resolveInputSocket(NodeID nodeID, 
                                          const std::string& socketName)
{
    return find<SocketID>(*_nodeTree, nodeID, 
        [&socketName](const InputSocketConfig& cfg) 
        {
            return socketName == cfg.name;
        });
}
    
SocketID NodeResolver::resolveInputSocket(const std::string& nodeName, 
                                          const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveInputSocket(nodeID, socketName);
    return InvalidSocketID;
}

const InputSocketConfig& NodeResolver::inputSocketConfig(NodeID nodeID, 
                                                         SocketID socketID)
{
    SocketID iter = 0;
    auto conf = find<const InputSocketConfig*>(*_nodeTree, nodeID, 
        [socketID,&iter](const InputSocketConfig& cfg)
        {
            return iter++ == socketID;
        });
    return conf ? *conf : invalidInputSocketConfig();
}

const InputSocketConfig& NodeResolver::inputSocketConfig(NodeID nodeID,
                                                         const std::string& socketName)
{
    auto conf = find<const InputSocketConfig*>(*_nodeTree, nodeID, 
        [&socketName](const InputSocketConfig& cfg)
        {
            return socketName == cfg.name;
        });
    return conf ? *conf : invalidInputSocketConfig();
}

const InputSocketConfig& NodeResolver::inputSocketConfig(const std::string& nodeName,
                                                         const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return inputSocketConfig(nodeID, socketName);
    return invalidInputSocketConfig();
}

SocketID NodeResolver::resolveOutputSocket(NodeID nodeID,
                                           const std::string& socketName)
{
    return find<SocketID>(*_nodeTree, nodeID, 
        [&socketName](const OutputSocketConfig& cfg)
        {
            return socketName == cfg.name;
        });
}

SocketID NodeResolver::resolveOutputSocket(const std::string& nodeName,
                                           const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveOutputSocket(nodeID, socketName);
    return InvalidSocketID;
}

const OutputSocketConfig& NodeResolver::outputSocketConfig(NodeID nodeID,
                                                           SocketID socketID)
{
    SocketID iter = 0;
    auto conf = find<const OutputSocketConfig*>(*_nodeTree, nodeID, 
        [socketID,&iter](const OutputSocketConfig& cfg)
        {
            return iter++ == socketID;
        });
    return conf ? *conf : invalidOutputSocketConfig();
}

const OutputSocketConfig& NodeResolver::outputSocketConfig(NodeID nodeID,
                                                           const std::string& socketName)
{
    auto conf = find<const OutputSocketConfig*>(*_nodeTree, nodeID, 
        [&socketName](const OutputSocketConfig& cfg)
        {
            return socketName == cfg.name;
        });
    return conf ? *conf : invalidOutputSocketConfig();
}

const OutputSocketConfig& NodeResolver::outputSocketConfig(const std::string& nodeName, 
                                                           const std::string& socketName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return outputSocketConfig(nodeID, socketName);
    return invalidOutputSocketConfig();
}

PropertyID NodeResolver::resolveProperty(NodeID nodeID, 
                                         const std::string& propertyName)
{
    return find<PropertyID>(*_nodeTree, nodeID, 
        [&propertyName](const PropertyConfig& cfg) 
        {
            return propertyName == cfg.name;
        });
}

PropertyID NodeResolver::resolveProperty(const std::string& nodeName,
                                         const std::string& propertyName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return resolveProperty(nodeID, propertyName);
    return InvalidPropertyID;
}

const PropertyConfig& NodeResolver::propertyConfig(NodeID nodeID, 
                                                   PropertyID propertyID)
{
    SocketID iter = 0;
    auto conf = find<const PropertyConfig*>(*_nodeTree, nodeID, 
        [propertyID,&iter](const PropertyConfig& cfg)
        {
            return iter++ == propertyID;
        });
    return conf ? *conf : invalidPropertyConfig();
}

const PropertyConfig& NodeResolver::propertyConfig(NodeID nodeID, 
                                                   const std::string& propertyName)
{
    auto conf = find<const PropertyConfig*>(*_nodeTree, nodeID, 
        [&propertyName](const PropertyConfig& cfg)
        {
            return propertyName == cfg.name;
        });
    return conf ? *conf : invalidPropertyConfig();
}

const PropertyConfig& NodeResolver::propertyConfig(const std::string& nodeName, 
                                                   const std::string& propertyName)
{
    NodeID nodeID;
    if((nodeID = resolveNode(nodeName)) != InvalidNodeID)
        return propertyConfig(nodeID, propertyName);
    return invalidPropertyConfig();
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
                    return{ nodeID, resolveInputSocket(nodeID, socket), false };
                else if (proto == "o")
                    return{ nodeID, resolveOutputSocket(nodeID, socket), true };
            }
        }
    }

    return{ InvalidNodeID, InvalidSocketID, false };
}

std::string NodeResolver::socketAddressUri(const SocketAddress& socketAddr)
{
    if(!socketAddr.isValid()) return{};

    std::ostringstream strm;

    if(socketAddr.isOutput)
    {
        strm << "o://";
        strm << _nodeTree->nodeName(socketAddr.node);
        strm << "/";
        strm << outputSocketConfig(socketAddr.node, socketAddr.socket).name;
    }
    else
    {
        strm << "i://";
        strm << _nodeTree->nodeName(socketAddr.node);
        strm << "/";
        strm << inputSocketConfig(socketAddr.node, socketAddr.socket).name;
    }    

    return strm.str();
}

namespace
{
    const PropertyConfig& invalidPropertyConfig()
    {
        static PropertyConfig instance{ EPropertyType::Unknown };
        return instance;
    }

    const InputSocketConfig& invalidInputSocketConfig()
    {
        static InputSocketConfig instance{ ENodeFlowDataType::Invalid };
        return instance;
    }

    const OutputSocketConfig& invalidOutputSocketConfig()
    {
        static OutputSocketConfig instance{ ENodeFlowDataType::Invalid };
        return instance;
    }

    // Returns the iterator (no-op)
    template <class RetValue, class Iter>
    RetValue return_value(Iter iter, Iter begin, typename std::enable_if<std::is_pointer<RetValue>::value>::type* = 0)
    {
        return iter;
    }

    // Returns the iterator's position
    template <class RetValue, class Iter>
    RetValue return_value(Iter iter, Iter begin, typename std::enable_if<!std::is_pointer<RetValue>::value>::type* = 0)
    {
        return static_cast<RetValue>(iter - begin);
    }

    // Type traits for SocketID/PropertyID
    template <class Value>
    struct NullValue;

    template <>
    struct NullValue<SocketID>
    {
        static const SocketID value = InvalidSocketID;
    };

    template <>
    struct NullValue<PropertyID>
    {
        static const PropertyID value = InvalidSocketID;
    };

    template <class RetValue>
    RetValue return_value_null(typename std::enable_if<std::is_pointer<RetValue>::value>::type* = 0)
    {
        return nullptr;
    }

    template <class RetValue>
    RetValue return_value_null(typename std::enable_if<!std::is_pointer<RetValue>::value>::type* = 0)
    {
        return NullValue<RetValue>::value;
    }

    template <class RetValue, class Pred>
    RetValue find(NodeTree& nodeTree, NodeID nodeID, Pred pred)
    {
        NodeConfig nodeConfig;
        if (nodeTree.nodeConfiguration(nodeID, nodeConfig))
        {
            //using PredArg = const InputSocketConfig&;
            using PredArg = func_traits<Pred>::arg<0>::type;
            // using Iter = InputSocketConfig
            using Iter = std::remove_cv<std::remove_reference<PredArg>::type>::type;

            const Iter* iter = begin_config<Iter>(nodeConfig);
            while (!end_config(iter))
            {
                if (pred(*iter))
                    return return_value<RetValue>(iter, begin_config<Iter>(nodeConfig));
                ++iter;
            }
        }

        return return_value_null<RetValue>();
    }
}