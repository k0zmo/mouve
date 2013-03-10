#pragma once

#include "Prerequisites.h"

enum class ENodeFlags
{
	Tagged             = BIT(1),
	StateNode          = BIT(2),
	AutoTag            = BIT(3),
	NotFullyConnected  = BIT(4)
};

class MOUVE_LOGIC_EXPORT Node
{
	Q_DISABLE_COPY(Node)
public:
	Node();
	Node(std::unique_ptr<NodeType> nodeType,
	     const std::string& nodeName,
	     NodeTypeID nodeTypeID);
	~Node();

	// Move operators
	Node(Node&& rhs);
	Node& operator=(Node&& rhs);

	// Is internal data valid 
	bool isValid() const;
	// Does given socket exists 
	bool validateSocket(SocketID socketID, bool isOutput) const;

	// Returns underlying NodeType implementation (supplied by the user)
	const std::unique_ptr<NodeType>& nodeType() const;

	NodeFlowData& outputSocket(SocketID socketID);
	const NodeFlowData& outputSocket(SocketID socketID) const;

	const std::string& nodeName() const;
	void setNodeName(const std::string& nodeName);

	SocketID numInputSockets() const;
	SocketID numOutputSockets() const;
	NodeTypeID nodeTypeID() const;

	// Below methods are thin wrapper for held NodeType interface
	void configuration(NodeConfig& nodeConfig) const;
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer);
	bool setProperty(PropertyID propID, const QVariant& value);
	QVariant property(PropertyID propID) const;
	bool initialize();

	bool flag(ENodeFlags flag) const;
	void setFlag(ENodeFlags flag);
	void unsetFlag(ENodeFlags flag);

private:
	// NOTE: If you add any new fields be sure to also handle them in move constructor/operator
	std::unique_ptr<NodeType> _nodeType;
	std::vector<NodeFlowData> _outputSockets;
	std::string _nodeName;
	SocketID _numInputs;
	SocketID _numOutputs;
	NodeTypeID _nodeTypeID;
	uint _flags;
};

class NodeIterator
{
public:
	virtual ~NodeIterator() {}
	virtual const Node* next(NodeID& nodeID) = 0;
};

inline bool Node::isValid() const
{ return static_cast<bool>(_nodeType); }

inline bool Node::validateSocket(SocketID socketID, bool isOutput) const
{ return isOutput ? (socketID < numOutputSockets()) : (socketID < numInputSockets()); }

inline const std::unique_ptr<NodeType>& Node::nodeType() const
{ return _nodeType; }

inline const std::string& Node::nodeName() const
{ return _nodeName; }

inline void Node::setNodeName(const std::string& nodeName)
{ _nodeName = nodeName; }

inline SocketID Node::numInputSockets() const
{ return _numInputs; }

inline SocketID Node::numOutputSockets() const
{ return _numOutputs; }

inline NodeTypeID Node::nodeTypeID() const
{ return _nodeTypeID; }

inline bool Node::flag(ENodeFlags flag) const
{ return _flags & uint(flag); }

inline void Node::setFlag(ENodeFlags flag)
{ _flags |= uint(flag); }

inline void Node::unsetFlag(ENodeFlags flag)
{ _flags &= ~uint(flag); }