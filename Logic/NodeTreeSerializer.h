#pragma once

#include "Prerequisites.h"

class QJsonObject;
class QJsonArray;

class LOGIC_EXPORT NodeTreeSerializer
{
public:
	NodeTreeSerializer(const std::string& rootDir = "");

	// Sets root directory - important for filepath properties
	void setRootDirectory(const std::string& rootDir);

	// Serializes given node tree to JSON object
	QJsonObject serializeJson(const std::shared_ptr<NodeTree>& nodeTree);

	// Deserializes given JSON object to nodeTree object
	// Returns true upon a success
	bool deserializeJson(std::shared_ptr<NodeTree>& nodeTree, 
		const QJsonObject& jsonTree, 
		std::map<NodeID, NodeID>* oldToNewNodeID = nullptr);

private:
	bool deserializeJsonNodes(std::shared_ptr<NodeTree>& nodeTree,
		const QJsonArray& jsonNodes,
		std::map<NodeID, NodeID>& mapping);
	bool deserializeJsonLinks(std::shared_ptr<NodeTree>& nodeTree,
		const QJsonArray& jsonLinks,
		std::map<NodeID, NodeID>& mapping);

private:
	std::string _rootDirectory;
};

