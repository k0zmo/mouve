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

std::string NodeType::formatMessage(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	const int big = 2048;
	static char output[big + 1] = {};

#if K_COMPILER == K_COMPILER_MSVC
	int len = vsnprintf_s(output, big, msg, args);
#else
	int len = vsnprintf(output, big, msg, args);
#endif
	if (len < 0 || len > big)
		len = big;
	output[len] = '\0';

	va_end(args);

	return std::string(output);
}