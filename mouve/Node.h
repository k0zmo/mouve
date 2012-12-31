#pragma once

#include "Prerequisites.h"

class Node
{
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

	cv::Mat& outputSocket(SocketID socketID);
	const cv::Mat& outputSocket(SocketID socketID) const;

	const std::string& nodeName() const;
	void setNodeName(const std::string& nodeName);

	SocketID numInputSockets() const;
	SocketID numOutputSockets() const;
	NodeTypeID nodeTypeID() const;

	// Below methods are thin wrapper for held NodeType interface
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer);
	void configuration(NodeConfig& nodeConfig) const;

private:
	// NOTE: If you add any new fields be sure to also handle them in move constructor/operator
	std::unique_ptr<NodeType> _nodeType;
	std::vector<cv::Mat> _outputSockets;
	std::string _nodeName;
	SocketID _numInputs;
	SocketID _numOutputs;
	NodeTypeID _nodeTypeID;

// gcc 4.7 doesn't play nice with this anymore, you need to use c++11 features
#if defined(Q_CC_MSVC)
private:
	Node(const Node&);
	Node& operator=(const Node&);
#elif defined(Q_CC_GNU)
public:
	Node(const Node&) = delete;
	Node& operator=(const Node&) = delete;
#endif
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
