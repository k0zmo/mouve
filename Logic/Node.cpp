#include "Node.h"
#include "NodeType.h"

/// xXx: Temporary here
#include <opencv2/core/core.hpp>

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
	, _numInputs(0)
	, _numOutputs(0)
	, _nodeTypeID(nodeTypeID)
{
	NodeConfig config = {0};
	_nodeType->configuration(config);

	// Count number of input sockets
	if(config.pInputSockets)
	{
		while(config.pInputSockets[_numInputs].name.length() > 0)
			_numInputs++;
	}

	// Count number of output sockets
	if(config.pOutputSockets)
	{
		while(config.pOutputSockets[_numOutputs].name.length() > 0)
			_numOutputs++;
	} 

	_outputSockets.resize(_numOutputs);
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
	}
	return *this;
}

cv::Mat& Node::outputSocket(SocketID socketID)
{
	Q_ASSERT(socketID < numOutputSockets());
	if(!validateSocket(socketID, true))
		throw std::runtime_error("bad socketID");

	return _outputSockets[socketID];
}

const cv::Mat& Node::outputSocket(SocketID socketID) const
{
	Q_ASSERT(socketID < numOutputSockets());
	if(!validateSocket(socketID, true))
		throw std::runtime_error("bad socketID");

	return _outputSockets[socketID];
}

void Node::execute(NodeSocketReader* reader, NodeSocketWriter* writer)
{
	writer->setOutputSockets(_outputSockets);
	_nodeType->execute(reader, writer);
}

void Node::configuration(NodeConfig& nodeConfig) const
{
	_nodeType->configuration(nodeConfig);
}

bool Node::property(PropertyID propID, const QVariant& value)
{
	return _nodeType->property(propID, value);
}