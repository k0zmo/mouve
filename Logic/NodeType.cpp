#include "NodeType.h"
#include "NodeTree.h"
#include "NodeLink.h"

/// xXx: Temporary here
#include <opencv2/core/core.hpp>

void NodeSocketReader::setNode(NodeID nodeID, SocketID numInputSockets)
{
	Q_ASSERT(nodeID != InvalidNodeID);
	_nodeID = nodeID;
	_numInputSockets = numInputSockets;
}

const cv::Mat& NodeSocketReader::readSocket(SocketID socketID) const
{
	Q_ASSERT(_nodeTree != nullptr);
	Q_ASSERT(_numInputSockets > 0);

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
			/// xXx: For now, just return empty matrix
			/// This should be taken care by NodeTree in near future! 
			static cv::Mat emptyMatrix = cv::Mat();
			return emptyMatrix;
		}
	}
	else
	{
		/// xXx: To throw or not to throw ?
		throw std::runtime_error("bad socket ID");
		//static cv::Mat invalidMat = cv::Mat();
		//return invalidMat;
	}
}

bool NodeSocketReader::allInputSocketsConnected() const
{
	Q_ASSERT(_nodeTree != nullptr);
	SocketID numConnected = 0;

	for(SocketID socketID = 0; socketID < _numInputSockets; ++socketID)
	{
		SocketAddress outputAddr = 
			_nodeTree->connectedFrom(SocketAddress(_nodeID, socketID, false));

		if(outputAddr.node != InvalidNodeID &&
		   outputAddr.socket != InvalidSocketID)
		{
			++numConnected;
		}
	}

	return numConnected == _numInputSockets;
}

void NodeSocketWriter::setOutputSockets(std::vector<cv::Mat>& outputs)
{
	_outputs = &outputs;
}

void NodeSocketWriter::writeSocket(SocketID socketID, cv::Mat& image)
{
	Q_ASSERT(_outputs != nullptr);
	Q_ASSERT(socketID < static_cast<int>(_outputs->size()));

	/// xXx: To throw or not to throw ?
	_outputs->at(socketID) = std::move(image);
}

cv::Mat& NodeSocketWriter::lockSocket(SocketID socketID)
{
	Q_ASSERT(_outputs != nullptr);
	Q_ASSERT(socketID < static_cast<int>(_outputs->size()));

	/// xXx: To throw or not to throw ?
	cv::Mat& output = _outputs->at(socketID);
	output = cv::Mat();

	return output;
}