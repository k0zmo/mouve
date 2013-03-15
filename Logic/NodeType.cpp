#include "NodeType.h"
#include "NodeTree.h"
#include "NodeLink.h"
#include "NodeException.h"

void NodeSocketReader::setNode(NodeID nodeID, SocketID numInputSockets)
{
	Q_ASSERT(nodeID != InvalidNodeID);
	_nodeID = nodeID;
	_numInputSockets = numInputSockets;
}

const NodeFlowData& NodeSocketReader::readSocket(SocketID socketID) const
{
	Q_ASSERT(_nodeTree != nullptr);
	Q_ASSERT(_numInputSockets > 0);
	Q_ASSERT(_nodeID != InvalidNodeID);

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
		throw node_bad_socket();
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
	Q_ASSERT(_outputs != nullptr);
	Q_ASSERT(socketID < static_cast<int>(_outputs->size()));

	if(socketID >= static_cast<int>(_outputs->size()))
		throw node_bad_socket();;

	_outputs->at(socketID) = std::move(image);
}

NodeFlowData& NodeSocketWriter::acquireSocket(SocketID socketID)
{
	Q_ASSERT(_outputs != nullptr);
	Q_ASSERT(socketID < static_cast<int>(_outputs->size()));

	if(socketID >= static_cast<int>(_outputs->size()))
		throw node_bad_socket();;

	return  _outputs->at(socketID);
}
