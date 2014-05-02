#pragma once

#include "Prerequisites.h"

class QJsonObject;
class QJsonArray;
class QVariant;

class LOGIC_EXPORT NodeTreeSerializer
{
public:
	NodeTreeSerializer(std::string rootDir = "")
		: _rootDirectory(std::move(rootDir))
	{}

	// Serializes given node tree to JSON object
	QJsonObject serializeJson(const std::shared_ptr<NodeTree>& nodeTree);

	bool deserializeJson(std::shared_ptr<NodeTree>& nodeTree, 
		const std::string& filePath);

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
		const std::map<NodeID, NodeID>& mapping);

	QJsonObject serializeProperty(PropertyID propID,
		const std::string& propName, EPropertyType propType,
		const NodeProperty& propValue);
	EPropertyType deserializePropertyType(const std::string&);
	NodeProperty deserializeProperty(EPropertyType propType, 
		const QVariant& qvalue);

private:
	std::string _rootDirectory;
};

