#pragma once

#include "Prerequisites.h"
#include "NodeFactory.h"

class NodeSystem
{
public:
	NodeSystem();
	~NodeSystem();

	NodeTypeID registerNodeType(const std::string& nodeTypeName,
	                            std::unique_ptr<NodeFactory> nodeFactory);
	std::unique_ptr<NodeType> createNode(NodeTypeID nodeTypeID) const;

	std::unique_ptr<NodeTree> createNodeTree();

	// Complementary method for NodeTypeID <---> NodeTypeName conversion
	const std::string& nodeTypeName(NodeTypeID nodeTypeID) const;
	NodeTypeID nodeTypeID(const std::string& nodeTypeName) const;

	std::unique_ptr<NodeTypeIterator> createNodeTypeIterator() const;

private:
	// That is all that NodeSystem needs for handling registered node types
	struct NodeTypeInfo
	{
		NodeTypeInfo(const std::string& nodeTypeName,
		             std::unique_ptr<NodeFactory> nodeFactory)
			: nodeTypeName(nodeTypeName)
			, nodeFactory(std::move(nodeFactory))
			, automaticallyRegistered(false)
		{}

		~NodeTypeInfo();

		std::string nodeTypeName;
		std::unique_ptr<NodeFactory> nodeFactory;
		bool automaticallyRegistered;

		// Mandatory when using unique_ptr
		NodeTypeInfo(NodeTypeInfo&& rhs);
		NodeTypeInfo& operator=(NodeTypeInfo&& rhs);

		// gcc 4.7 doesn't play nice with this anymore, you need to use c++11 features
	#if defined(Q_CC_MSVC)
	private:
		NodeTypeInfo(const NodeTypeInfo&);
		NodeTypeInfo& operator=(const NodeTypeInfo&);
	#elif defined(Q_CC_GNU)
		NodeTypeInfo(const NodeTypeInfo&) = delete;
		NodeTypeInfo& operator=(const NodeTypeInfo&) = delete;
	#endif
	};

	std::vector<NodeTypeInfo> _registeredNodeTypes;
	std::unordered_map<std::string, NodeTypeID> _typeNameToTypeID;

private:
	// Disallows copying/cloning of NodeSystem
	NodeSystem(const NodeSystem&);
	NodeSystem& operator=(const NodeSystem&);

private:
	class NodeTypeIteratorImpl;

private:
	// This makes it easy for nodes to be registered automatically
	void registerAutoTypes();
};

