#include "NodeSystem.h"
#include "NodeType.h"
#include "NodeTree.h"

/// xXx: Change this to some neat logging system
#include <iostream>

static const std::string InvalidType("InvalidType");

NodeSystem::NodeSystem()
{
	// Register InvalidNodeTypeID
	NodeTypeID invalidNodeTypeID = registerNodeType(InvalidType, nullptr);
	Q_ASSERT(invalidNodeTypeID == InvalidNodeTypeID);
	registerAutoTypes();
}

NodeSystem::~NodeSystem()
{
}

NodeTypeID NodeSystem::registerNodeType(const std::string& nodeTypeName,
	std::unique_ptr<NodeFactory> nodeFactory)
{
	// Check if we haven't already registed so named node
	auto iter = _typeNameToTypeID.find(nodeTypeName);
	if(iter == _typeNameToTypeID.end())
	{
		/// xXx: Should we protect ourselves from too much node types being registered?

		// New type of node
		size_t nodeTypeID = _registeredNodeTypes.size();
		_typeNameToTypeID[nodeTypeName] = NodeTypeID(nodeTypeID);

		// Associate type name with proper factory
		_registeredNodeTypes.emplace_back(NodeTypeInfo(nodeTypeName, std::move(nodeFactory)));

		return NodeTypeID(nodeTypeID);
	}
	else
	{
		/// xXx: return invalid typeID or just override previous one?
		NodeTypeID nodeTypeID = iter->second;

		std::cout << "NodeSystem::registerNodeType: Type '" << nodeTypeName << "' Id='"
			<< int(nodeTypeID) << "' is already registed, overriding with a new factory\n";

		// Override previous factory with a new one
		_registeredNodeTypes[nodeTypeID].nodeFactory = std::move(nodeFactory);

		return nodeTypeID;
	}
}

std::unique_ptr<NodeType> NodeSystem::createNode(NodeTypeID nodeTypeID) const
{
	// Check if the type exists
	Q_ASSERT(nodeTypeID < _registeredNodeTypes.size());
	if(nodeTypeID >= _registeredNodeTypes.size())
		return nullptr;

	// Retrieve proper factory
	auto& nodeFactory = _registeredNodeTypes[nodeTypeID].nodeFactory;
	if(nodeFactory != nullptr)
	{
		return nodeFactory->create();
	}
	else
	{
		return nullptr;
	}
}

std::unique_ptr<NodeTree> NodeSystem::createNodeTree()
{
	return std::unique_ptr<NodeTree>(new NodeTree(this));
}

const std::string& NodeSystem::nodeTypeName(NodeTypeID nodeTypeID) const
{
	Q_ASSERT(nodeTypeID < _registeredNodeTypes.size());
	if(nodeTypeID >= _registeredNodeTypes.size())
		return InvalidType;
	return _registeredNodeTypes[nodeTypeID].nodeTypeName;
}

NodeTypeID NodeSystem::nodeTypeID(const std::string& nodeTypeName) const
{
	auto iter = _typeNameToTypeID.find(nodeTypeName);
	if(iter == _typeNameToTypeID.end())
	{
		return InvalidNodeTypeID;
	}
	else
	{
		return iter->second;
	}
}

void NodeSystem::registerAutoTypes()
{
	AutoRegisterNodeBase* autoFactory = AutoRegisterNodeBase::head;
	while(autoFactory)
	{
		auto nodeFactory = std::unique_ptr<NodeFactory>(autoFactory);
		NodeTypeID nodeTypeID = registerNodeType(autoFactory->typeName, std::move(nodeFactory));
		autoFactory = autoFactory->next;

		// We need empty deleter as autoFactory
		// are allocated on (global) stack.
		// We could get rid of unique_ptr at all
		// but that would lead to less friendly
		// interface for manually registered node types.
		// Unfortunately, we can't mixed unique_ptrs with 
		// default deleters and overridden by us
		_registeredNodeTypes[nodeTypeID].automaticallyRegistered = true;
	}
}

// -----------------------------------------------------------------------------

NodeSystem::NodeTypeInfo::~NodeTypeInfo()
{
	if(automaticallyRegistered)
	{
		// This should stop unique_ptr from calling deleter
		(void) nodeFactory.release();
	}
}

NodeSystem::NodeTypeInfo::NodeTypeInfo(NodeSystem::NodeTypeInfo&& rhs)
{
	operator=(std::forward<NodeSystem::NodeTypeInfo>(rhs));
}

NodeSystem::NodeTypeInfo& NodeSystem::NodeTypeInfo::operator=(NodeSystem::NodeTypeInfo&& rhs)
{
	nodeTypeName = std::move(rhs.nodeTypeName);
	nodeFactory = std::move(rhs.nodeFactory);
	automaticallyRegistered = rhs.automaticallyRegistered;

	return *this;
}

// -----------------------------------------------------------------------------

class NodeSystem::NodeTypeIteratorImpl : public NodeTypeIterator
{
public:
	NodeTypeIteratorImpl(const std::vector<NodeSystem::NodeTypeInfo>& registeredNodeTypes)
		: _nodeTypeID(InvalidNodeTypeID)
		, _registeredNodeTypes(registeredNodeTypes)
		, _iterator(registeredNodeTypes.cbegin())
	{
		// Iterator points to first valid type info
		if(_iterator != _registeredNodeTypes.cend())
			++_iterator;
	}

	virtual bool next(NodeTypeIterator::NodeTypeInfo& nodeType)
	{
		if(_iterator == _registeredNodeTypes.cend())
			return false;

		nodeType.typeID = ++_nodeTypeID;
		nodeType.typeName = _iterator->nodeTypeName;

		++_iterator;
		return true;
	}

private:
	int _nodeTypeID;
	const std::vector<NodeSystem::NodeTypeInfo>& _registeredNodeTypes;
	std::vector<NodeSystem::NodeTypeInfo>::const_iterator _iterator;
};

std::unique_ptr<NodeTypeIterator> NodeSystem::createNodeTypeIterator() const
{
	return std::unique_ptr<NodeTypeIterator>(new NodeTypeIteratorImpl(_registeredNodeTypes));
}