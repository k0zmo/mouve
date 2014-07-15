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

#include "Node.h"
#include "NodeType.h"
#include "NodeProperty.h"
#include "NodeException.h"

HighResolutionClock Node::_stopWatch;

Node::Node()
    : _nodeType(nullptr)
    , _outputSockets(0)
    , _nodeName()
    , _numInputs(0)
    , _numOutputs(0)
    , _nodeTypeID(InvalidNodeTypeID)
    , _flags(0)
    , _timeElapsed(0)
{
}

Node::Node(std::unique_ptr<NodeType> nodeType,
           const std::string& nodeName,
           NodeTypeID nodeTypeID)
    : _nodeType(std::move(nodeType))
    , _nodeName(nodeName)
    , _numInputs(0)
    , _numOutputs(0)
    , _nodeTypeID(nodeTypeID)
    , _flags(0)
    , _timeElapsed(0)
{
    NodeConfig config;
    _nodeType->configuration(config);

    if(config.flags.testFlag(ENodeConfig::HasState))
        setFlag(ENodeFlags::StateNode);
    if(config.flags.testFlag(ENodeConfig::AutoTag))
        setFlag(ENodeFlags::AutoTag);
    if(config.flags.testFlag(ENodeConfig::OverridesTimeComputation))
        setFlag(ENodeFlags::OverridesTimeComp);

    // Count number of input sockets
    auto input = begin_config<InputSocketConfig>(config);
    while(!end_config(input))
    {
        ++_numInputs;
        ++input;
    }

    _outputSockets.clear();

    // Count number of output sockets
    auto output = begin_config<OutputSocketConfig>(config);
    while(!end_config(output))
    {
        _outputSockets.emplace_back(output->dataType);
        ++output;
    }

    _numOutputs = static_cast<SocketID>(_outputSockets.size());
}

Node::~Node()
{
}

Node::Node(Node&& rhs)
{
    operator=(std::forward<Node>(rhs));
}

Node& Node::operator=(Node&& rhs)
{ 
    if(&rhs != this)
    {
        _outputSockets = std::move(rhs._outputSockets);
        _nodeType = std::move(rhs._nodeType);
        _nodeName = std::move(rhs._nodeName);
        _numInputs = rhs._numInputs;
        _numOutputs = rhs._numOutputs;
        _nodeTypeID = rhs._nodeTypeID;
        _flags = rhs._flags;
        _timeElapsed = rhs._timeElapsed;
        _message = rhs._message;
    }
    return *this;
}

NodeFlowData& Node::outputSocket(SocketID socketID)
{
    assert(socketID < numOutputSockets());
    if(!validateSocket(socketID, true))
        throw BadSocketException();

    return _outputSockets[socketID];
}

const NodeFlowData& Node::outputSocket(SocketID socketID) const
{
    assert(socketID < numOutputSockets());
    if(!validateSocket(socketID, true))
        throw BadSocketException();

    return _outputSockets[socketID];
}

void Node::configuration(NodeConfig& nodeConfig) const
{
    _nodeType->configuration(nodeConfig);
}

ExecutionStatus Node::execute(NodeSocketReader& reader, NodeSocketWriter& writer)
{
    writer.setOutputSockets(_outputSockets);
    double start = _stopWatch.currentTimeInSeconds();
    ExecutionStatus status = _nodeType->execute(reader, writer);
    double stop = _stopWatch.currentTimeInSeconds();

    _message = status.message;
    _timeElapsed = !flag(ENodeFlags::OverridesTimeComp)
        ? (stop - start) * 1e3 // Save in milliseconds
        : status.timeElapsed;

    return status;
}

bool Node::setProperty(PropertyID propID, const NodeProperty& value)
{
    return _nodeType->setProperty(propID, value);
}

NodeProperty Node::property(PropertyID propID) const
{
    return _nodeType->property(propID);
}

const std::string& Node::executeInformation() const
{
    return _message;
}

bool Node::restart()
{
    return _nodeType->restart();
}

void Node::finish()
{
    _nodeType->finish();
}
