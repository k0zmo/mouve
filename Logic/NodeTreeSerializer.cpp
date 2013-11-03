#include "NodeTreeSerializer.h"
#include "NodeTree.h"
#include "NodeType.h"
#include "NodeSystem.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDebug>

NodeTreeSerializer::NodeTreeSerializer(const std::string& rootDir)
	: _rootDirectory(rootDir)
{
}

void NodeTreeSerializer::setRootDirectory(const std::string& rootDir)
{
	_rootDirectory = rootDir;
}

QJsonObject NodeTreeSerializer::serializeJson(const std::shared_ptr<NodeTree>& nodeTree)
{
	// Iterate over all nodes and serialize it as JSON value of JSON array
	QJsonArray jsonNodes;
	auto nodeIt = nodeTree->createNodeIterator();
	NodeID nodeID;
	while(const Node* node = nodeIt->next(nodeID))
	{
		QJsonObject jsonNode;

		QString typeName = QString::fromStdString(nodeTree->nodeTypeName(nodeID));
		QString name = QString::fromStdString(node->nodeName());

		jsonNode.insert(QStringLiteral("id"), nodeID);
		jsonNode.insert(QStringLiteral("class"), typeName);
		jsonNode.insert(QStringLiteral("name"), name);

		// Save properties' values
		NodeConfig nodeConfig;
		node->configuration(nodeConfig);
		PropertyID propID = 0;
		QJsonArray jsonProperties;

		if(nodeConfig.pProperties)
		{
			while(nodeConfig.pProperties[propID].type != EPropertyType::Unknown)
			{
				QVariant propValue = node->property(propID);

				if(propValue.isValid())
				{
					const auto& prop = nodeConfig.pProperties[propID];

					QJsonObject jsonProp;
					jsonProp.insert(QStringLiteral("id"), propID);
					jsonProp.insert(QStringLiteral("name"), QString::fromStdString(prop.name));

					switch(prop.type)
					{
					case EPropertyType::Boolean:
						jsonProp.insert(QStringLiteral("value"), propValue.toBool());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("boolean"));
						break;
					case EPropertyType::Integer:
						jsonProp.insert(QStringLiteral("value"), propValue.toInt());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("integer"));
						break;
					case EPropertyType::Double:
						jsonProp.insert(QStringLiteral("value"), propValue.toDouble());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("double"));
						break;
					case EPropertyType::Enum:
						jsonProp.insert(QStringLiteral("value"), propValue.toInt());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("enum"));
						break;
					case EPropertyType::Matrix:
						{
							QJsonArray jsonMatrix;
							Matrix3x3 matrix = propValue.value<Matrix3x3>();
							for(double v : matrix.v)
								jsonMatrix.append(v);
							jsonProp.insert(QStringLiteral("value"), jsonMatrix);
							jsonProp.insert(QStringLiteral("type"), QStringLiteral("matrix3x3"));
							break;
						}
					case EPropertyType::Filepath:
						{
							QString rootDirStr = QString::fromStdString(_rootDirectory);
							QDir rootDir(rootDirStr);
							if(rootDir.exists())
								jsonProp.insert(QStringLiteral("value"), rootDir.relativeFilePath(propValue.toString()));
							else
								jsonProp.insert(QStringLiteral("value"), propValue.toString());
							jsonProp.insert(QStringLiteral("type"), QStringLiteral("filepath"));
						}
						break;
					case EPropertyType::String:
						jsonProp.insert(QStringLiteral("value"), propValue.toString());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("string"));
						break;
					case EPropertyType::Unknown:
						break;
					}

					jsonProperties.append(jsonProp);
				}

				++propID;
			}
		}

		if(!jsonProperties.isEmpty())
			jsonNode.insert(QStringLiteral("properties"), jsonProperties);

		jsonNodes.append(jsonNode);
	}

	// Iterate over all links and serialize it as JSON value of JSON array
	QJsonArray jsonLinks;
	auto linkIt = nodeTree->createNodeLinkIterator();
	NodeLink nodeLink;
	while(linkIt->next(nodeLink))
	{
		QJsonObject jsonLink;

		jsonLink.insert(QStringLiteral("fromNode"), nodeLink.fromNode);
		jsonLink.insert(QStringLiteral("fromSocket"), nodeLink.fromSocket);
		jsonLink.insert(QStringLiteral("toNode"), nodeLink.toNode);
		jsonLink.insert(QStringLiteral("toSocket"), nodeLink.toSocket);

		jsonLinks.append(jsonLink);
	}

	// Finally, add JSON arrays representing nodes and links
	QJsonObject jsonTree;
	jsonTree.insert(QStringLiteral("nodes"), jsonNodes);
	jsonTree.insert(QStringLiteral("links"), jsonLinks);
	return jsonTree;
}


bool NodeTreeSerializer::deserializeJson(std::shared_ptr<NodeTree>& nodeTree,
										 const QJsonObject& jsonTree, 
										 std::map<NodeID, NodeID>* oldToNewNodeID)
{
	nodeTree->clear();

	QJsonArray nodes = jsonTree["nodes"].toArray();
	QJsonArray links = jsonTree["links"].toArray();

	/// TODO : check if the "nodes", "links" are indeed QVariantList?
	std::map<NodeID, NodeID> mapping;

	if(!deserializeJsonNodes(nodeTree, nodes, mapping))
	{
		qCritical("Didn't load all saved nodes, tree will be discarded");
		nodeTree->clear();
		return false;
	}
	if(!deserializeJsonLinks(nodeTree, links, mapping))
	{
		qCritical("Didn't load all saved links, tree will be discarded");
		nodeTree->clear();
		return false;
	}

	if(oldToNewNodeID != nullptr)
		*oldToNewNodeID = mapping;

	return true;
}

bool NodeTreeSerializer::deserializeJsonNodes(std::shared_ptr<NodeTree>& nodeTree,
											  const QJsonArray& jsonNodes,
											  std::map<NodeID, NodeID>& mapping)
{
	QVariantList nodes = jsonNodes.toVariantList();
	int i = 0;

	// Load nodes
	for(const auto& node : nodes)
	{
		QVariantMap mapNode = node.toMap();		

		bool ok;
		NodeID nodeId = mapNode["id"].toUInt(&ok);
		if(!ok)
		{
			qCritical() << "\"id\" is bad";
			break;
		}

		QString nodeTypeName = mapNode["class"].toString();
		if(nodeTypeName.isEmpty())
		{
			qCritical() << QString("\"class\" is bad (id:%1)").arg(nodeId);;
			break;
		}

		QString nodeName = mapNode["name"].toString();
		if(nodeName.isEmpty())
		{
			qCritical() << QString("\"name\" is bad (id:%1)").arg(nodeId);;
			break;
		}

		// Try to create new node 
		NodeTypeID nodeTypeId = nodeTree->nodeSystem()->nodeTypeID(nodeTypeName.toStdString());
		NodeID _nodeId = nodeTree->createNode(nodeTypeId, nodeName.toStdString());
		if(_nodeId == InvalidNodeID)
		{
			qCritical() << QString("Couldn't create node of id: %1 and type name: %3")
				.arg(nodeId)
				.arg(nodeTypeName);
			break;
		}
		mapping.insert(std::make_pair(nodeId, _nodeId));

		QVariant varProperties = mapNode["properties"];
		if(varProperties.isValid() 
			&& varProperties.type() == QVariant::List)
		{
			QVariantList properties = varProperties.toList();

			for(const auto& prop : properties)
			{
				QVariantMap propMap = prop.toMap();

				bool ok;
				uint propId = propMap["id"].toUInt(&ok);
				if(!ok)
				{
					qWarning() << "\"id\" is bad, property will be skipped";
					continue;
				}

				QVariant value = propMap["value"];
				if(!value.isValid())
				{
					qWarning() << "\"value\" is bad, property of id" << propId << "will be skipped";
					continue;
				}

				// Special case - type is matrix3x3. For rest types we don't even check it (at least for now)
				QString propType = propMap["type"].toString();
				if(propType == "matrix3x3")
				{
					// Convert from QList<QVariant (of double)> to Matrix3x3
					QVariantList matrixList = value.toList();
					Matrix3x3 matrix;
					for(int i = 0; i < matrixList.count() && i < 9; ++i)
						matrix.v[i] = matrixList[i].toDouble();

					value = QVariant::fromValue<Matrix3x3>(matrix);
				}

				if(propType == "filepath")
				{
					QString rootDirStr = QString::fromStdString(_rootDirectory);
					QDir rootDir(rootDirStr);
					if(rootDir.exists())
						value = rootDir.absoluteFilePath(value.toString());
				}

				if(!nodeTree->nodeSetProperty(_nodeId, propId, value))
				{
					qWarning() << "Couldn't set loaded property" << propId <<  "to" << value;
				}
			}
		}

		++i;
	}

	return nodes.count() == i;
}

bool NodeTreeSerializer::deserializeJsonLinks(std::shared_ptr<NodeTree>& nodeTree,
											  const QJsonArray& jsonLinks,
											  std::map<NodeID, NodeID>& mapping)
{
	int i = 0;
	QVariantList links = jsonLinks.toVariantList();

	for(const auto& link : links)
	{
		QVariantMap mapLink = link.toMap();
		bool ok;

		NodeID fromNode = mapLink["fromNode"].toUInt(&ok);
		if(!ok)
		{
			qCritical() << "\"fromNode\" is bad";
			break;
		}
		SocketID fromSocket = mapLink["fromSocket"].toUInt(&ok);
		if(!ok)
		{
			qCritical() << "\"fromSocket\" is bad";
			break;
		}
		NodeID toNode = mapLink["toNode"].toUInt(&ok);
		if(!ok)
		{
			qCritical() << "\"toNode\" is bad";
			break;
		}
		SocketID toSocket = mapLink["toSocket"].toUInt(&ok);
		if(!ok)
		{
			qCritical() << "\"toSocket\" is bad";
			break;
		}

		if(nodeTree->linkNodes(SocketAddress(mapping[fromNode], fromSocket, true),
		                       SocketAddress(mapping[toNode], toSocket, false)) != ELinkNodesResult::Ok)
		{
			qCritical() << QString("Couldn't link nodes %1:%2 with %3:%4")
				.arg(fromNode)
				.arg(fromSocket)
				.arg(toNode)
				.arg(toSocket);
			break;
		}
		++i;
	}

	return links.count() == i;
}
