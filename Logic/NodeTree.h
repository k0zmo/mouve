#pragma once

#include "Prerequisites.h"
#include "Node.h"
#include "NodeLink.h"

class MOUVE_LOGIC_EXPORT NodeTree
{
	Q_DISABLE_COPY(NodeTree)
public:
	NodeTree(NodeSystem* nodeSystem);
	~NodeTree();

	void clear();

	void tagNode(NodeID nodeID);
	void untagNode(NodeID nodeID);
	void tagAutoNodes();
	bool isTreeStateless() const;

	void prepareList();
	void execute(bool withInit = false);
	
	// Returns sorted list of node in order of their execution
	// Returned list is valid between prepareList() calls.
	std::vector<NodeID> executeList() const;

	NodeID createNode(NodeTypeID typeID, const std::string& name);
	bool removeNode(NodeID nodeID);
	bool removeNode(const std::string& nodeName);

	bool linkNodes(SocketAddress from, SocketAddress to);
	bool unlinkNodes(SocketAddress from, SocketAddress to);

	// Set a new name for a given node
	void setNodeName(NodeID nodeID, const std::string& newNodeName);
	// Returns the current node name for a given node
	const std::string& nodeName(NodeID nodeID) const;

	// For a given a nodeName returns NodeID
	NodeID resolveNode(const std::string& nodeName) const;
	// For a given NodeID returns its type ID
	NodeTypeID nodeTypeID(NodeID nodeID) const;
	// For a given NodeID returns its type name as string
	const std::string& nodeTypeName(NodeID nodeID) const;	

	// Returns output socket address from which given input socket is connected to
	SocketAddress connectedFrom(SocketAddress iSocketAddress) const;
	const cv::Mat& outputSocket(NodeID nodeID, SocketID socketID) const;
	const cv::Mat& inputSocket(NodeID nodeID, SocketID socketID) const;

	// Returns true if given input socket is connected 
	bool isInputSocketConnected(NodeID nodeID, SocketID socketID) const;
	// Returns true if given output socket is connected
	bool isOutputSocketConnected(NodeID nodeID, SocketID socketID) const;

	bool nodeConfiguration(NodeID nodeID, NodeConfig& nodeConfig) const;
	bool nodeSetProperty(NodeID nodeID, PropertyID propID, const QVariant& value);
	QVariant nodeProperty(NodeID nodeID, PropertyID propID);

	std::unique_ptr<NodeIterator> createNodeIterator();
	std::unique_ptr<NodeLinkIterator> createNodeLinkIterator();	

private:
	NodeID allocateNodeID();
	void deallocateNodeID(NodeID id);

	size_t firstOutputLink(NodeID fromNode, SocketID fromSocket, size_t start = 0) const;
	void addToExecuteList(std::vector<NodeID>& execList, NodeID nodeID);
	void traverseRecurs(std::vector<NodeID>& execList, NodeID nodeID);

	bool validateLink(SocketAddress& from, SocketAddress& to);
	bool validateNode(NodeID nodeID) const;

private:
	std::vector<Node> _nodes;
	std::vector<NodeID> _recycledIDs;
	std::vector<NodeID> _taggedNodesID;
	std::vector<NodeLink> _links;
	std::vector<NodeID> _executeList;
	std::unordered_map<std::string, NodeID> _nodeNameToNodeID;
	std::set<NodeID> _stateNodes; // assuming small size of it
	std::vector<NodeID> _autoTaggedNodes;
	NodeSystem* _nodeSystem;
	bool _executeListDirty;

private:
	// Interfaces implementations
	class NodeIteratorImpl;
	class NodeLinkIteratorImpl;
};
