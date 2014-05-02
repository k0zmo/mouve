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

const cv::Mat& NodeSocketReader::readSocketImage(SocketID socketID) const
{
    return readSocket(socketID).getImage();
}

void NodeSocketWriter::setOutputSockets(std::vector<NodeFlowData>& outputs)
{
    _outputs = &outputs;
}

void NodeSocketWriter::writeSocket(SocketID socketID, NodeFlowData&& image)
{
    assert(_outputs != nullptr);
    assert(socketID < static_cast<int>(_outputs->size()));

    if(socketID >= static_cast<int>(_outputs->size()))
        throw BadSocketException();;

    _outputs->at(socketID) = std::move(image);
}

NodeFlowData& NodeSocketWriter::acquireSocket(SocketID socketID)
{
    assert(_outputs != nullptr);
    assert(socketID < static_cast<int>(_outputs->size()));

    if(socketID >= static_cast<int>(_outputs->size()))
        throw BadSocketException();;

    return _outputs->at(socketID);
}