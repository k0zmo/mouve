#include "Node.h"
#include "NodeType.h"

Node::Node()
	: _nodeType(nullptr)
	, _outputSockets(0)
	, _nodeName()
	, _numInputs(0)
	, _numOutputs(0)
{
}

Node::Node(std::unique_ptr<NodeType> nodeType,
           const std::string& nodeName)
	: _nodeType(std::move(nodeType))
	, _nodeName(nodeName)
{
	/// xXx: Temporary - NodeConfiguration
	_numInputs = _nodeType->numInputSockets();
	_numOutputs = _nodeType->numOutputSockets();

	_outputSockets.resize(_numOutputs);
}

Node::Node(Node&& rhs)
{
	operator=(std::move(rhs));
}

Node& Node::operator=(Node&& rhs)
{ 
	_outputSockets = std::move(rhs._outputSockets);
	_nodeType = std::move(rhs._nodeType);

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
