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

	if(config.flags & Node_HasState)
		setFlag(ENodeFlags::StateNode);
	if(config.flags & Node_AutoTag)
		setFlag(ENodeFlags::AutoTag);
	if(config.flags & Node_OverridesTimeComputation)
		setFlag(ENodeFlags::OverridesTimeComp);

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
		_outputSockets.emplace_back(config.pOutputSockets[i].dataType);
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
