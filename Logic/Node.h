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

#include <chrono>

enum class ENodeFlags : int
{
    Tagged             = 1 << 1,
    StateNode          = 1 << 2,
    AutoTag            = 1 << 3,
    NotFullyConnected  = 1 << 4,
    OverridesTimeComp  = 1 << 5,
    Disabled           = 1 << 6
};

class Node
{
public:
    Node();
    Node(std::unique_ptr<NodeType> nodeType,
         const std::string& nodeName,
         NodeTypeID nodeTypeID);

    Node(const Node& rhs) = delete;
    Node& operator=(const Node& rhs) = delete;

    Node(Node&& rhs) = default;
    Node& operator=(Node&& rhs) = default;

    // Is internal data valid
    bool isValid() const;
    // Does given socket exists
    bool validateSocket(SocketID socketID, bool isOutput) const;

    // Returns underlying NodeType implementation (supplied by the user)
    const std::unique_ptr<NodeType>& nodeType() const;

    NodeFlowData& outputSocket(SocketID socketID);
    const NodeFlowData& outputSocket(SocketID socketID) const;

    const std::string& nodeName() const;
    void setNodeName(const std::string& nodeName);

    SocketID numInputSockets() const;
    SocketID numOutputSockets() const;
    NodeTypeID nodeTypeID() const;
    std::chrono::high_resolution_clock::duration timeElapsed() const;

    // Below methods are thin wrapper for held NodeType interface
    const NodeConfig& config() const;
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer);
    bool setProperty(PropertyID propID, const NodeProperty& value);
    NodeProperty property(PropertyID propID) const;
    const std::string& executeInformation() const;
    bool restart();
    void finish();

    bool flag(ENodeFlags flag) const;
    void setFlag(ENodeFlags flag);
    void unsetFlag(ENodeFlags flag);

private:
    std::unique_ptr<NodeType> _nodeType;
    std::vector<NodeFlowData> _outputSockets;
    std::string _nodeName;
    SocketID _numInputs;
    SocketID _numOutputs;
    NodeTypeID _nodeTypeID;
    uint32_t _flags;
    std::chrono::high_resolution_clock::duration _timeElapsed;
    std::string _message;
};

class NodeIterator
{
public:
    virtual ~NodeIterator() {}
    virtual const Node* next(NodeID& nodeID) = 0;
};

inline bool Node::isValid() const
{ return static_cast<bool>(_nodeType); }

inline bool Node::validateSocket(SocketID socketID, bool isOutput) const
{ return isOutput ? (socketID < numOutputSockets()) : (socketID < numInputSockets()); }

inline const std::unique_ptr<NodeType>& Node::nodeType() const
{ return _nodeType; }

inline const std::string& Node::nodeName() const
{ return _nodeName; }

inline void Node::setNodeName(const std::string& nodeName)
{ _nodeName = nodeName; }

inline SocketID Node::numInputSockets() const
{ return _numInputs; }

inline SocketID Node::numOutputSockets() const
{ return _numOutputs; }

inline NodeTypeID Node::nodeTypeID() const
{ return _nodeTypeID; }

inline std::chrono::high_resolution_clock::duration Node::timeElapsed() const
{ return _timeElapsed; }

inline bool Node::flag(ENodeFlags flag) const
{ return (_flags & uint32_t(flag)) != 0; }

inline void Node::setFlag(ENodeFlags flag)
{ _flags |= uint32_t(flag); }

inline void Node::unsetFlag(ENodeFlags flag)
{ _flags &= ~uint32_t(flag); }
