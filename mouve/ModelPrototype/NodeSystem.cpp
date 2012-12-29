#include "NodeSystem.h"
#include "NodeType.h"

/// xXx: Change this to some neat logging system
#include <iostream>

static const std::string InvalidType("InvalidType");

NodeSystem::NodeSystem()
{
	// Register InvalidNodeTypeID
	NodeTypeID invalidNodeTypeID = registerNodeType(InvalidType, nullptr);
	ASSERT(invalidNodeTypeID == InvalidNodeTypeID);
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
	ASSERT(nodeTypeID < _registeredNodeTypes.size());
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

const std::string& NodeSystem::nodeTypeName(NodeTypeID nodeTypeID) const
{
	ASSERT(nodeTypeID < _registeredNodeTypes.size());
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

// -----------------------------------------------------------------------------

NodeSystem::NodeTypeInfo::NodeTypeInfo(NodeSystem::NodeTypeInfo&& rhs)
{
	operator=(std::forward<NodeSystem::NodeTypeInfo>(rhs));
}

NodeSystem::NodeTypeInfo& NodeSystem::NodeTypeInfo::operator=(NodeSystem::NodeTypeInfo&& rhs)
{
	nodeTypeName = std::move(rhs.nodeTypeName);
	nodeFactory = std::move(rhs.nodeFactory);

	return *this;
}