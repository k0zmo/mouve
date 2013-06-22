#pragma once

#include "Prerequisites.h"
#include "NodeFactory.h"

class LOGIC_EXPORT NodeSystem
{
	K_DISABLE_COPY(NodeSystem)
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

	bool registerNodeModule(const std::shared_ptr<NodeModule>& module);
	const std::shared_ptr<NodeModule>& nodeModule(const std::string& name);

	void loadPlugin(const std::string& pluginName);

private:
	// That is all that NodeSystem needs for handling registered node types
	class NodeTypeInfo
	{
		K_DISABLE_COPY(NodeTypeInfo)
	public:
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
	};

	std::vector<NodeTypeInfo> _registeredNodeTypes;
	std::unordered_map<std::string, NodeTypeID> _typeNameToTypeID;
	std::unordered_map<std::string, std::shared_ptr<NodeModule>> _registeredModules;
	std::unordered_map<std::string, std::unique_ptr<NodePlugin>> _plugins;

private:
	class NodeTypeIteratorImpl;

private:
	// This makes it easy for nodes to be registered automatically
	void registerAutoTypes();
};

