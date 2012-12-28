#pragma once

#include "Prerequisites.h"

// TODO
enum ETagReason
{
	Reason_Unspecified,
	Reason_NewFrame,
	Reason_AttributeChanged,
	Reason_ConnectionChanged
};

class NodeTree
{
public:
	NodeTree(/*std::shared_ptr<NodeSystem> nodeSyst*/);
	~NodeTree();

	void clear();
	void step();

	void tagNode(NodeID nodeID, ETagReason reason = Reason_Unspecified);

	NodeID createNode(NodeTypeID typeID, const std::string& name);
	bool removeNode(NodeID);
	//bool removeNode(const std::string& nodeName);

	bool linkNodes(SocketAddress from, SocketAddress to);
	bool unlinkNodes(SocketAddress from, SocketAddress to);

	bool validateLink(SocketAddress from, SocketAddress to);

	// Zwraca socket wyjsciowy do ktorego podlaczony jest podany adres
	SocketAddress connectedFrom(SocketAddress iSocketAddress) const;
	const cv::Mat& outputSocket(NodeID nodeID, SocketID socketID) const;
	const cv::Mat& inputSocket(NodeID nodeID, SocketID socketID) const;

	bool isInputSocketConnected(NodeID nodeID, SocketID socketID) const;
	bool isOutputSocketConnected(NodeID nodeID, SocketID socketID) const;

	//std::unique_ptr<NodeIterator> createNodeIterator();
    std::unique_ptr<NodeLinkIterator> createNodeLinkIterator();

private:
	NodeID allocateNodeID();
	void deallocateNodeID(NodeID id);

	size_t firstOutputLink(NodeID fromNode, SocketID fromSocket, size_t start = 0) const;
	void addToExecuteList(std::vector<NodeID>& execList, NodeID nodeID);
	void traverseRecurs(std::vector<NodeID>& execList, NodeID nodeID);

private:
	std::vector<Node> _nodes;
	std::vector<NodeID> _recycledIDs;
	std::vector<NodeID> _taggedNodesID;

	//std::unordered_map<std::string, NodeID> _nodeNameToNodeID;

	std::vector<NodeLink> _links;

private:
	// Interfaces implementations
	class NodeIterator;
	class NodeLinkIterator;
};
