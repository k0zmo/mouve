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

struct InputSocketConfig
{
	std::string name;
	std::string humanName;
	std::string description;
	//NodeSocketData defaultData;
};

struct OutputSocketConfig
{
	std::string name;
	std::string humanName;
	std::string description;
	//int type;
};

struct NodeConfig
{
	const InputSocketConfig* pInputSockets;
	const OutputSocketConfig* pOutputSockets;
	std::string description;
};

class NodeType
{
public:
	virtual ~NodeType() {}
	// virtual void initialize();
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer) = 0;
	virtual void configuration(NodeConfig& nodeConfig) const = 0;
	// void registerUpdateInterval();
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