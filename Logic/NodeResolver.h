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

class LOGIC_EXPORT NodeResolver
{
public:
    NodeResolver(std::shared_ptr<NodeTree> nodeTree);

    // For input socket:  i://nodename/socketname
    // For output socket: o://nodename/socketname
    // For node property: p://nodename/propertyname
    int resolve(const std::string& uri);

    NodeID resolveNode(const std::string& nodeName);

    SocketID resolveInputSocket(NodeID nodeID, 
                                const std::string& socketName);
    SocketID resolveInputSocket(const std::string& nodeName,
                                const std::string& socketName);
    const InputSocketConfig& inputSocketConfig(NodeID nodeID,
                                               SocketID socketID);
    const InputSocketConfig& inputSocketConfig(NodeID nodeID,
                                               const std::string& socketName);
    const InputSocketConfig& inputSocketConfig(const std::string& nodeName,
                                               const std::string& socketName);

    SocketID resolveOutputSocket(NodeID nodeID, 
                                 const std::string& socketName);
    SocketID resolveOutputSocket(const std::string& nodeName,
                                 const std::string& socketName);
    const OutputSocketConfig& outputSocketConfig(NodeID nodeID,
                                                 SocketID socketID);
    const OutputSocketConfig& outputSocketConfig(NodeID nodeID,
                                                 const std::string& socketName);
    const OutputSocketConfig& outputSocketConfig(const std::string& nodeName,
                                                 const std::string& socketName);

    PropertyID resolveProperty(NodeID nodeID, 
                               const std::string& propertyName);
    PropertyID resolveProperty(const std::string& nodeName,
                               const std::string& propertyName);
    const PropertyConfig& propertyConfig(NodeID nodeID, PropertyID propertyID);
    const PropertyConfig& propertyConfig(NodeID nodeID,
                                         const std::string& propertyName);
    const PropertyConfig& propertyConfig(const std::string& nodeName,
                                         const std::string& propertyName);

    SocketAddress resolveSocketAddress(const std::string& uri);
    std::string socketAddressUri(const SocketAddress& socketAddr);

private:
    std::shared_ptr<NodeTree> _nodeTree;
};