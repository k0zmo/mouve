#include "NodeTree.h"
#include "Node.h"
#include "NodeType.h"
#include "NodeLink.h"
#include "NodeSystem.h"

/// xXx: Change this to some neat logging system
#include <iostream>

NodeTree::NodeTree(NodeSystem* nodeSystem)
	: _nodeSystem(nodeSystem)
{
}

NodeTree::~NodeTree()
{
	// TODO: clean up ...

	clear();
}

void NodeTree::clear()
{
	_nodes.clear();
	_recycledIDs.clear();
	_taggedNodesID.clear();
	_links.clear();
	_nodeNameToNodeID.clear();
}

void NodeTree::step()
{
	// stable_sort doesn't touch already sorted items (?)
	//std::sort(std::begin(links), std::end(links));
	std::stable_sort(std::begin(_links), std::end(_links));

	/// xXx: remove duplicates? or just dont allow to create it?

	// Build execution list
	std::vector<NodeID> execList;

	//for(NodeID nodeID : _taggedNodesID)
	for(auto it = _taggedNodesID.begin(); it != _taggedNodesID.end(); ++it)
	{
		NodeID nodeID = *it;

		if(!validateNode(nodeID))
			continue;

		if(std::find(execList.begin(), execList.end(), nodeID) == execList.end())
		{
			addToExecuteList(execList, nodeID);
			traverseRecurs(execList, nodeID);
		}		
	}

	// DEBUG
	std::cout << "NodeID execution list:\n";
	//for(NodeID nodeID : execList)
	for(auto it = execList.begin(); it != execList.end(); ++it)
	{
		NodeID nodeID = *it;
		std::cout << nodeID << " ";
	}
	std::cout << "\n";

	NodeSocketReader reader(this);
	NodeSocketWriter writer;

	// Traverse through just-built exec list and process each node
	//for(NodeID nodeID : execList)
	for(auto it = execList.begin(); it != execList.end(); ++it)
	{
		NodeID nodeID = *it;
		auto& nodeRef = _nodes[nodeID];

		reader.setNode(nodeID, nodeRef.numInputSockets());
		nodeRef.execute(&reader, &writer);
	}
}

void NodeTree::tagNode(NodeID nodeID, ETagReason reason)
{
	// TODO
	(void) reason;

	if(std::find(_taggedNodesID.begin(), 
	        _taggedNodesID.end(), nodeID) == _taggedNodesID.end())
	{
		_taggedNodesID.push_back(nodeID);
	}
	else
	{
		std::cout << "Node " << nodeID << " already tagged, skipping\n";
	}
}

NodeID NodeTree::createNode(NodeTypeID typeID, const std::string& name)
{
	// Check if the given name is unique
	if(_nodeNameToNodeID.find(name) != _nodeNameToNodeID.end())
		return InvalidNodeID;

	if(!_nodeSystem)
		return InvalidNodeID;

	// Allocate NodeID
	NodeID id = allocateNodeID();
	if(id == InvalidNodeID)
		return id;

	// Create node (type) of a given type
	std::unique_ptr<NodeType> nodeType = _nodeSystem->createNode(typeID);
	if(nodeType == nullptr)
	{
		deallocateNodeID(id);
		return InvalidNodeID;
	}

	// Create actual node object
	_nodes[id] = Node(std::move(nodeType), name, typeID);
	// Add the pair <node name, node ID> to hash map
	_nodeNameToNodeID.insert(std::make_pair(name, id));
	return id;
}

bool NodeTree::removeNode(NodeID nodeID)
{
	if(!validateNode(nodeID))
		return false;

	// Remove any referring links
	auto removeFirstNodeLink = std::remove_if(
		std::begin(_links), std::end(_links),
		[&](const NodeLink& nodeLink)
		{
			return nodeLink.fromNode == nodeID ||
			       nodeLink.toNode   == nodeID;
		});
	/// xXx: tag affected nodes?
	_links.erase(removeFirstNodeLink, std::end(_links));

	deallocateNodeID(nodeID);

	return false;
}

bool NodeTree::removeNode(const std::string& nodeName)
{
	auto iter = _nodeNameToNodeID.find(nodeName);
	if(iter == _nodeNameToNodeID.end())
		return false;

	return removeNode(iter->second);
}

bool NodeTree::linkNodes(SocketAddress from, SocketAddress to)
{
	if(!validateLink(from, to))
		return false;
	_links.emplace_back(NodeLink(from, to));

	/// xXx: tag affected nodes?

	return true;
}

bool NodeTree::unlinkNodes(SocketAddress from, SocketAddress to)
{
	if(!validateLink(from, to))
		return false;

	// lower_bound requires container to be sorted
	std::stable_sort(std::begin(_links), std::end(_links));

	// look for given node link
	NodeLink nodeLinkToFind(from, to);
	auto iter = std::lower_bound(std::begin(_links), 
		std::end(_links), nodeLinkToFind);

	if(iter != std::end(_links))
	{
		_links.erase(iter);

		/// xXx: tag affected nodes?

		return true;
	}

	return false;
}

void NodeTree::setNodeName(NodeID nodeID, const std::string& newNodeName)
{
	if(nodeID < _nodes.size())
	{
		auto& node = _nodes[nodeID];
		if(node.isValid() && node.nodeName() != newNodeName)
		{
			// Remove old mapping
			_nodeNameToNodeID.erase(node.nodeName());

			// Change a name and and new mapping
			node.setNodeName(newNodeName);
			_nodeNameToNodeID.insert(std::make_pair(newNodeName, nodeID));
		}
	}
}

const std::string& NodeTree::nodeName(NodeID nodeID) const
{
	static const std::string InvalidNodeStr("InvalidNode");
	static const std::string InvalidNodeIDStr("InvalidNodeID");

	if(nodeID < _nodes.size())
	{
		auto& node = _nodes[nodeID];
		if(node.isValid())
		{
			return node.nodeName();
		}
		return InvalidNodeStr;
	}
	return InvalidNodeIDStr;
}

NodeID NodeTree::resolveNode(const std::string& nodeName) const
{
	auto iter = _nodeNameToNodeID.find(nodeName);
	if(iter != _nodeNameToNodeID.end())
	{
		return iter->second;
	}
	else
	{
		return InvalidNodeID;
	}
}

NodeTypeID NodeTree::nodeTypeID(NodeID nodeID) const
{
	if(!validateNode(nodeID))
		return InvalidNodeTypeID;
	return _nodes[nodeID].nodeTypeID();
}

const std::string& NodeTree::nodeTypeName(NodeID nodeID) const
{
	if(_nodeSystem != nullptr)
	{
		if(nodeID < _nodes.size())
		{
			NodeTypeID nodeTypeID = _nodes[nodeID].nodeTypeID();
			return _nodeSystem->nodeTypeName(nodeTypeID);
		}
	}

	static const std::string EmptyString = "";
	return EmptyString;
}

SocketAddress NodeTree::connectedFrom(SocketAddress iSocketAddr) const
{
	// This function does not rely on a assumption that a links are sorted
	SocketAddress ret(InvalidNodeID, InvalidSocketID, false);

	// ... and only works when addr points to input socket
	if(iSocketAddr.isOutput)
		return ret;

	//for(const NodeLink& link: _links)
	for(auto it = _links.cbegin(); it != _links.cend(); ++it)
	{
		const NodeLink& link = *it;

		if(link.toNode == iSocketAddr.node &&
		   link.toSocket == iSocketAddr.socket)
		{
			ret.node = link.fromNode;
			ret.socket = link.fromSocket;
			ret.isOutput = true;
			break;
		}
	}

	return ret;
}

const cv::Mat& NodeTree::outputSocket(NodeID nodeID, SocketID socketID) const
{
	if(!validateNode(nodeID))
		/// xXx: To throw or not to throw ?
		throw std::runtime_error("node validation failed");

	// outputSocket verifies socketID
	return _nodes[nodeID].outputSocket(socketID);
}

const cv::Mat& NodeTree::inputSocket(NodeID nodeID, SocketID socketID) const
{
	if(!validateNode(nodeID))
		/// xXx: To throw or not to throw ?
		throw std::runtime_error("node validation failed");

	SocketAddress outputAddr = connectedFrom(
		SocketAddress(nodeID, socketID, false));
	// outputSocket verifies outputAddr
	return outputSocket(outputAddr.node, outputAddr.socket);
}

bool NodeTree::isInputSocketConnected(NodeID nodeID, SocketID socketID) const
{
	SocketAddress outputAddr = connectedFrom(
		SocketAddress(nodeID, socketID, false));
	return outputAddr.isValid();
}

bool NodeTree::isOutputSocketConnected(NodeID nodeID, SocketID socketID) const
{
	return firstOutputLink(nodeID, socketID) != size_t(-1);
}

bool NodeTree::nodeConfiguration(NodeID nodeID, NodeConfig& nodeConfig) const
{
	if(!validateNode(nodeID))
		return false;

	_nodes[nodeID].configuration(nodeConfig);
	return true;
}

NodeID NodeTree::allocateNodeID()
{
	NodeID id = InvalidNodeID;

	// Check for recycled ids
	if(_recycledIDs.empty())
	{
		// Need to create new one
		id = NodeID(_nodes.size());
		// Expand node array - createNode relies on it
		_nodes.resize(_nodes.size() + 1); 
	}
	else
	{
		// Reuse unused but still cached one
		id = _recycledIDs.back();
		_recycledIDs.pop_back();
	}

	return id;
}

void NodeTree::deallocateNodeID(NodeID id)
{
	if(!validateNode(id))
		return;

	// Remove mapping from node name to node ID
	_nodeNameToNodeID.erase(_nodes[id].nodeName());

	// Invalidate node
	_nodes[id] = Node();

	// Add NodeID to recycled ones
	_recycledIDs.push_back(id);
}

size_t NodeTree::firstOutputLink(NodeID fromNode,
	SocketID fromSocket, size_t start) const
{
	for(size_t i = start; i < _links.size(); ++i)
	{
		auto& link = _links[i];

		if(link.fromNode == fromNode &&
		   link.fromSocket == fromSocket)
		{
			return i;
		}
	}
	return size_t(-1);
}

void NodeTree::addToExecuteList(std::vector<NodeID>& execList, NodeID nodeID)
{
	// Check if the given nodeID is already on the list
	for(auto iter = execList.begin(); iter != execList.end(); ++iter)
	{
		if(*iter == nodeID)
		{
			// If so, we need to "move it" to last position
			execList.erase(iter);
			break;
		}
	}

	// If not, just append it at the end of the list
	execList.push_back(nodeID);
}

void NodeTree::traverseRecurs(std::vector<NodeID>& execList, NodeID nodeID)
{
	SocketID numOutputSockets = _nodes[nodeID].numOutputSockets();
	SocketID lastSocketID = 0;

	for(SocketID socketID = 0; socketID < numOutputSockets; ++socketID)
	{
		// Find first output link from this node and socket (can be more than 1)
		size_t firstOutputLinkIndex = firstOutputLink
			(nodeID, socketID, lastSocketID);
		if(firstOutputLinkIndex == size_t(-1))
			continue;
		lastSocketID = firstOutputLinkIndex;

		auto iter    = std::begin(_links) + firstOutputLinkIndex;
		auto iterEnd = std::end(_links);

		while(iter != iterEnd &&
			  iter->fromNode == nodeID &&
			  iter->fromSocket == socketID)
		{
			addToExecuteList(execList, iter->toNode);
			traverseRecurs(execList, iter->toNode);
			++iter;
		}
	}
}

bool NodeTree::validateLink(SocketAddress& from, SocketAddress& to)
{
	if(from.isOutput == to.isOutput)
		return false;

	// A little correction
	if(!from.isOutput)
		std::swap(from, to);

	if(!validateNode(from.node) || 
	   !_nodes[from.node].validateSocket(from.socket, from.isOutput))
		return false;

	if(!validateNode(to.node) ||
	   !_nodes[to.node].validateSocket(to.socket, to.isOutput))
		return false;

	return true;
}

bool NodeTree::validateNode(NodeID nodeID) const
{
	if(nodeID >= _nodes.size() || nodeID == InvalidNodeID)
		return false;
	return _nodes[nodeID].isValid();
}

// -----------------------------------------------------------------------------

class NodeTree::NodeIteratorImpl : public NodeIterator
{
public:
	NodeIteratorImpl(NodeTree* parent)
		: _parent(parent)
		, _iter(parent->_nodeNameToNodeID.begin())
	{}

	virtual const Node* next(NodeID& nodeID)
	{
		Node* out = nullptr;
		nodeID = InvalidNodeID;

		if(_iter != _parent->_nodeNameToNodeID.end())
		{
			nodeID = _iter->second;
			out = &_parent->_nodes[nodeID];
			++_iter;
		}

		return out;
	}

private:
	NodeTree* _parent;
	std::unordered_map<std::string, NodeID>::iterator _iter;
};

std::unique_ptr<NodeIterator> NodeTree::createNodeIterator()
{
	return std::unique_ptr<NodeIterator>(new NodeIteratorImpl(this));
}

class NodeTree::NodeLinkIteratorImpl : public NodeLinkIterator
{
public:
	NodeLinkIteratorImpl(NodeTree* parent)
		: _parent(parent)
		, _iter(parent->_links.begin())
	{}

	virtual bool next(NodeLink& nodeLink)
	{
		if(_parent)
		{
			if(_iter == _parent->_links.end())
				return false;

			nodeLink.fromNode = _iter->fromNode;
			nodeLink.fromSocket = _iter->fromSocket;
			nodeLink.toNode = _iter->toNode;
			nodeLink.toSocket = _iter->toSocket;

			++_iter;
		}
		return true;
	}

private:
	NodeTree* _parent;
	std::vector<NodeLink>::iterator _iter;
};

std::unique_ptr<NodeLinkIterator> NodeTree::createNodeLinkIterator()
{
	return std::unique_ptr<NodeLinkIterator>(new NodeLinkIteratorImpl(this));
}
