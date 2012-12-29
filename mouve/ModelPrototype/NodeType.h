#pragma once

#include "Prerequisites.h"

class NodeSocketReader
{
public:
	NodeSocketReader(NodeTree* tree)
		: _nodeTree(tree)
		, _numInputSockets(0)
		, _nodeID(InvalidNodeID)
	{}

	/// xXx: make it private? and friend with nodetree/node?
	void setNode(NodeID nodeID, SocketID numInputSockets);

	const cv::Mat& readSocket(SocketID socketID) const;
	bool allInputSocketsConnected() const;

private:
	NodeTree* _nodeTree;
	SocketID _numInputSockets;
	NodeID _nodeID;

private:
	NodeSocketReader(const NodeSocketReader&);
	NodeSocketReader& operator=(const NodeSocketReader&);
};

class NodeSocketWriter
{
public:
	NodeSocketWriter()
		: _outputs(nullptr)
	{}

	/// xXx: make it private? and friend with nodetree/node?
	void setOutputSockets(std::vector<cv::Mat>& outputs);

	void writeSocket(SocketID socketID, cv::Mat& image);
	cv::Mat& lockSocket(SocketID socketID);

private:
	std::vector<cv::Mat>* _outputs;

private:
	NodeSocketWriter(const NodeSocketWriter&);
	NodeSocketWriter& operator=(const NodeSocketWriter&);
};

class NodeType
{
public:
	virtual ~NodeType() {}
	// virtual void initialize();
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer) = 0;
	// void registerUpdateInterval();

	// temporary - should be gone when Configuration is introduced
	virtual SocketID numInputSockets() const = 0;
	virtual SocketID numOutputSockets() const = 0;
};

class NodeTypeIterator
{
public:
	struct NodeTypeInfo
	{
		NodeTypeID typeID;
		std::string typeName;
	};

	virtual ~NodeTypeIterator() {} 
	virtual bool next(NodeTypeInfo& nodeTypeInfo) = 0;
};