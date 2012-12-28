#pragma once

#include "Prerequisites.h"

class NodeIterator
{
public:
	virtual bool next(NodeID& node) = 0;
};

class Node
{
public:
	Node();
	Node(std::unique_ptr<NodeType> nodeType,
	     const std::string& nodeName);

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

	// Below methods are thin wrapper for held NodeType interface
	/// xXx: should be different meaning but same effect (based on NodeConfiguration, not yet another method)
	SocketID numOutputSockets() const;
	SocketID numInputSockets() const;
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer);

private:
	/// xXx: Copy operators, for now disabled
	Node(const Node& rhs);
	Node& operator=(const Node& rhs);

private:
	std::unique_ptr<NodeType> _nodeType;
	std::vector<cv::Mat> _outputSockets;
	std::string _nodeName;
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