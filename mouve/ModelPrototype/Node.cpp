#include "Node.h"
#include "NodeType.h"

Node::Node()
	: _nodeType(nullptr)
	, _outputSockets(0)
	, _nodeName()
	, _numInputs(0)
	, _numOutputs(0)
	, _nodeTypeID(InvalidNodeTypeID)
{
}

Node::Node(std::unique_ptr<NodeType> nodeType,
           const std::string& nodeName,
           NodeTypeID nodeTypeID)
	: _nodeType(std::move(nodeType))
	, _nodeName(nodeName)
	, _nodeTypeID(nodeTypeID)
{
	/// xXx: Temporary - NodeConfiguration
	_numInputs = _nodeType->numInputSockets();
	_numOutputs = _nodeType->numOutputSockets();

	_outputSockets.resize(_numOutputs);
}

Node::Node(Node&& rhs)
{
	operator=(std::forward<Node>(rhs));
}

Node& Node::operator=(Node&& rhs)
{ 
	_outputSockets = std::move(rhs._outputSockets);
	_nodeType = std::move(rhs._nodeType);
	_nodeName = std::move(rhs._nodeName);
	_numInputs = rhs._numInputs;
	_numOutputs = rhs._numOutputs;
	_nodeTypeID = rhs._nodeTypeID;
	return *this;
}

cv::Mat& Node::outputSocket(SocketID socketID)
{
	ASSERT(socketID < numOutputSockets());

	/// xXx: To throw or not to throw ?
	return _outputSockets[socketID];
}

const cv::Mat& Node::outputSocket(SocketID socketID) const
{
	ASSERT(socketID < numOutputSockets());

	/// xXx: To throw or not to throw ?
	return _outputSockets[socketID];
}

void Node::execute(NodeSocketReader* reader, NodeSocketWriter* writer)
{
	writer->setOutputSockets(_outputSockets);
	_nodeType->execute(reader, writer);
}
