#include "NodeTree.h"
#include "Node.h"
#include "NodeType.h"
#include "NodeLink.h"

/// xXx: Change this to some neat logging system
#include <iostream>

NodeTree::NodeTree(/*std::shared_ptr<NodeSystem> nodeSyst*/)
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

void NodeTree::step()
{
	// stable_sort doesn't touch already sorted items (?)
	//std::sort(std::begin(links), std::end(links));
	std::stable_sort(std::begin(_links), std::end(_links));

	/// xXx: remove duplicates? or just dont allow to create it?

	// Build execution list
	std::vector<NodeID> execList;

	for(NodeID nodeID : _taggedNodesID)
	{
		if(std::find(execList.begin(), execList.end(), nodeID) == execList.end())
		{
			addToExecuteList(execList, nodeID);
			traverseRecurs(execList, nodeID);
		}		
	}

	// DEBUG
	for(NodeID nodeID : execList)
	{
		std::cout << nodeID << " ";
	}
	std::cout << "\n";

	NodeSocketReader reader(this);
	NodeSocketWriter writer;

	// Traverse through just-built exec list and process each node
	for(NodeID nodeID : execList)
	{
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

// MOCK
#include "BuiltinNodeTypes.h"
std::unique_ptr<NodeType> createNodeType(NodeTypeID typeID)
{
	switch(typeID)
	{
	case 1:
		return std::unique_ptr<NodeType>(new ImageFromFileNodeType());
	case 2:
		return std::unique_ptr<NodeType>(new GaussianBlurNodeType());
	case 3:
		return std::unique_ptr<NodeType>(new CannyEdgeDetector());
	default:
		return std::unique_ptr<NodeType>(nullptr);
	}

	//switch(typeID)
	//{
	//case 1:  return std::unique_ptr<NodeType>(new ZeroInputOneOutputNodeType());
	//case 2:  return std::unique_ptr<NodeType>(new ZeroInputTwoOutputNodeType());
	//case 3:  return std::unique_ptr<NodeType>(new OneInputOneOutputNodeType());
	//case 4:  return std::unique_ptr<NodeType>(new OneInputTwoOutputNodeType());
	//case 5:  return std::unique_ptr<NodeType>(new TwoInputOneOutputNodeType());
	//case 6:  return std::unique_ptr<NodeType>(new TwoInputZeroOutputNodeType());
	//default: return std::unique_ptr<NodeType>(nullptr);
	//}
}

NodeID NodeTree::createNode(NodeTypeID typeID, const std::string& name)
{
	// Check if the given name is unique
	if(_nodeNameToNodeID.find(name) != _nodeNameToNodeID.end())
		return InvalidNodeID;

	// Allocate NodeID
	NodeID id = allocateNodeID();
	if(id == InvalidNodeID)
		return id;

	// Create node (type) of a given type
	std::unique_ptr<NodeType> nodeType = createNodeType(typeID);
	if(nodeType == nullptr)
	{
		deallocateNodeID(id);
		return InvalidNodeID;
	}

	// Create actual node object
	_nodes[id] = Node(std::move(nodeType), name);
	// Add the pair <node name, node ID> to hash map
	_nodeNameToNodeID.insert(std::make_pair(name, id));
	return id;
}

bool NodeTree::removeNode(NodeID)
{
	return false;
}

//bool NodeTree::removeNode(const std::string& nodeName);

bool NodeTree::linkNodes(SocketAddress from, SocketAddress to)
{
	if(!validateLink(from, to))
		return false;
	_links.emplace_back(NodeLink(from, to));
	return true;
}

bool NodeTree::unlinkNodes(SocketAddress from, SocketAddress to)
{
	(void) from;
	(void) to;

	// TODO
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

bool NodeTree::validateLink(SocketAddress from, SocketAddress to)
{
	(void) from;
	(void) to;

	return true;
}

SocketAddress NodeTree::connectedFrom(SocketAddress iSocketAddr) const
{
	// This function does not rely on a assumption that a links are sorted
	SocketAddress ret(InvalidNodeID, InvalidSocketID, false);

	// ... and only works when addr points to input socket
	if(iSocketAddr.isOutput)
		return ret;

	for(const NodeLink& link: _links)
	{
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
	//if(!validateNode(nodeID))
	//	return nullptr;

	const Node& nodeRef = _nodes[nodeID];
	//if(ref.validateSocket(socket, true))
	//	return nullptr;

	return nodeRef.outputSocket(socketID);
}

const cv::Mat& NodeTree::inputSocket(NodeID nodeID, SocketID socketID) const
{
	//if(!validateNode(nodeID))
	//	return nullptr;

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
//	if(!validateNode(id))
//		return;

	// Remove mapping from node name to node ID
	_nodeNameToNodeID.erase(_nodes[id].nodeName());

	// Invalidate node
	_nodes[id] = Node();

	// Add NodeID to recycled ones
	_recycledIDs.push_back(id);
}

// -----------------------------------------------------------------------------

//class NodeTree::NodeIterator : public ::NodeIterator
//{
//public:
//	NodeIterator(NodeTree* parent)
//		: _parent(parent)
//		, _iter(parent->mNodeNameToTypeId.begin())
//	{ }
//
//	virtual INodeInternal* next(TNodeId& id)
//	{
//		INodeInternal* out = nullptr;
//
//		if(iter != parent->mNodeNameToTypeId.end())
//		{
//			id = iter->second;
//			out = &parent->mNodeInternals[id];
//			++iter;
//		}
//
//		return out;
//	}
//
//private:
//	NodeTree* parent;
//	unordered_map<string, TNodeId>::iterator iter;
//};
//
//std::unique_ptr<NodeIterator> NodeTree::createNodeIterator()
//{
//}

class NodeTree::NodeLinkIterator : public ::NodeLinkIterator
{
public:
	NodeLinkIterator(NodeTree* parent)
		: _parent(parent)
		, _iter(parent->_links.begin())
	{ }

	virtual bool next(NodeLink& link) // this is not NodeTree::Link
	{
		if(_iter == _parent->_links.end())
			return false;

		link.fromNode = _iter->fromNode;
		link.fromSocket = _iter->fromSocket;
		link.toNode = _iter->toNode;
		link.toSocket = _iter->toSocket;

		++_iter;

		return true;
	}

private:
	NodeTree* _parent;
	std::vector<NodeLink>::iterator _iter;
};

std::unique_ptr<NodeLinkIterator> NodeTree::createNodeLinkIterator()
{
	return std::unique_ptr<::NodeLinkIterator>(new NodeLinkIterator(this));
}