#include "Node.h"
#include "NodeType.h"
#include "NodeException.h"

Node::Node()
	: _nodeType(nullptr)
	, _outputSockets(0)
	, _nodeName()
	, _numInputs(0)
	, _numOutputs(0)
	, _nodeTypeID(InvalidNodeTypeID)
	, _flags(0)
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
{
	NodeConfig config;
	_nodeType->configuration(config);

	if(config.flags & Node_HasState)
		setFlag(ENodeFlags::StateNode);
	if(config.flags & Node_AutoTag)
		setFlag(ENodeFlags::AutoTag);

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

	_outputSockets.clear();
	for(int i = 0; i < _numOutputs; ++i)
	{
		_outputSockets.emplace_back(ENodeFlowDataType::Image, cv::Mat());
	}
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
	}
	return *this;
}

NodeFlowData& Node::outputSocket(SocketID socketID)
{
	Q_ASSERT(socketID < numOutputSockets());
	if(!validateSocket(socketID, true))
		throw node_bad_socket();

	return _outputSockets[socketID];
}

const NodeFlowData& Node::outputSocket(SocketID socketID) const
{
	Q_ASSERT(socketID < numOutputSockets());
	if(!validateSocket(socketID, true))
		throw node_bad_socket();

	return _outputSockets[socketID];
}

void Node::configuration(NodeConfig& nodeConfig) const
{
	_nodeType->configuration(nodeConfig);
}

ExecutionStatus Node::execute(NodeSocketReader& reader, NodeSocketWriter& writer)
{
	writer.setOutputSockets(_outputSockets);
	return _nodeType->execute(reader, writer);
}

bool Node::setProperty(PropertyID propID, const QVariant& value)
{
	return _nodeType->setProperty(propID, value);
}

QVariant Node::property(PropertyID propID) const
{
	return _nodeType->property(propID);
}

bool Node::initialize()
{
	return _nodeType->initialize();
}