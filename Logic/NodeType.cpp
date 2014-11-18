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

#include "NodeType.h"
#include "NodeTree.h"
#include "NodeLink.h"
#include "NodeException.h"

void NodeSocketReader::setNode(NodeID nodeID, SocketID numInputSockets)
{
    assert(nodeID != InvalidNodeID);
    _nodeID = nodeID;
    _numInputSockets = numInputSockets;
}

const NodeFlowData& NodeSocketReader::readSocket(SocketID socketID) const
{
    assert(_nodeTree != nullptr);
    assert(_numInputSockets > 0);
    assert(_nodeID != InvalidNodeID);

    if(socketID < _numInputSockets)
    {
        SocketAddress outputAddr = 
            _nodeTree->connectedFrom(SocketAddress(_nodeID, socketID, false));
        _tracer.setSocket(socketID, false);

        if(outputAddr.isValid())
        {
            return _nodeTree->outputSocket(outputAddr.node, outputAddr.socket);
        }
        // Socket is not connected
        else
        {
            // Return an empty NodeFlowData
            static NodeFlowData _default = NodeFlowData();
            return _default;
        }
    }
    else
    {
        throw BadSocketException();
    }
}

void NodeSocketWriter::setOutputSockets(std::vector<NodeFlowData>& outputs)
{
    _outputs = &outputs;
}

NodeFlowData& NodeSocketWriter::acquireSocket(SocketID socketID)
{
    assert(_outputs != nullptr);
    assert(socketID < static_cast<int>(_outputs->size()));

    if(socketID >= static_cast<int>(_outputs->size()))
        throw BadSocketException();;

    _tracer.setSocket(socketID, true);
    return _outputs->at(socketID);
}

bool PropertyConfig::setPropertyValue(const NodeProperty& newPropertyValue)
{
    if(_validator)
    {
        if(!_validator->validate(newPropertyValue))
            return false;
    }

    _nodeProperty = newPropertyValue;

    if(_observer)
        _observer->notifyChanged(newPropertyValue);

    return true;
}

NodeConfig::NodeConfig(NodeConfig&& other)
{
    *this = std::move(other);
}

NodeConfig& NodeConfig::operator=(NodeConfig&& other)
{
    if (&other != this)
    {
        _inputs = std::move(other._inputs);
        _outputs = std::move(other._outputs);
        _properties = std::move(other._properties);
        _description = std::move(other._description);
        _module = std::move(other._module);
        _flags = other._flags;
    }

    return *this;
}

SocketConfig& NodeConfig::addInput(std::string name, ENodeFlowDataType dataType)
{
    if(dataType == ENodeFlowDataType::Invalid)
        throw BadConfigException();
    if(!checkNameUniqueness(_inputs, name))
        throw BadConfigException();

    _inputs.emplace_back(static_cast<SocketID>(_inputs.size()), std::move(name), dataType);
    return _inputs.back();
}

void NodeConfig::clearInputs()
{
    _inputs.clear();
}

SocketConfig& NodeConfig::addOutput(std::string name, ENodeFlowDataType dataType)
{
    if(dataType == ENodeFlowDataType::Invalid)
        throw BadConfigException();
    if(!checkNameUniqueness(_outputs, name))
        throw BadConfigException();

    _outputs.emplace_back(static_cast<SocketID>(_outputs.size()), std::move(name), dataType);
    return _outputs.back();
}

void NodeConfig::clearOutputs()
{
    _outputs.clear();
}

PropertyConfig& NodeConfig::addProperty(std::string name, NodeProperty& nodeProperty)
{
    if(!nodeProperty.isValid())
        throw BadConfigException();
    if(!checkNameUniqueness(_properties, name))
        throw BadConfigException();

    _properties.emplace_back(static_cast<PropertyID>(_properties.size()), std::move(name), nodeProperty);
    return _properties.back();
}

void NodeConfig::clearProperties()
{
    _properties.clear();
}

template <class T>
bool NodeConfig::checkNameUniqueness(const std::vector<T>& containter,
                            const std::string& name)
{
    return std::find_if(std::begin(containter), std::end(containter),
        [&name](const T& cfg)
        {
            return cfg.name() == name;
        }) == std::end(containter);
}