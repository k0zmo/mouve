#include "NodeTreeSerializer.h"
#include "NodeTree.h"
#include "NodeType.h"
#include "NodeSystem.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDebug>

QJsonObject NodeTreeSerializer::serializeJson(const std::shared_ptr<NodeTree>& nodeTree)
{
	// Iterate over all nodes and serialize it as JSON value of JSON array
	QJsonArray jsonNodes;
	auto nodeIt = nodeTree->createNodeIterator();
	NodeID nodeID;
	while(const Node* node = nodeIt->next(nodeID))
	{
		QJsonObject jsonNode;

		jsonNode.insert(QStringLiteral("id"), nodeID);
		jsonNode.insert(QStringLiteral("class"), QString::fromStdString(nodeTree->nodeTypeName(nodeID)));
		jsonNode.insert(QStringLiteral("name"), QString::fromStdString(node->nodeName()));

		// Save properties' values if any
		NodeConfig nodeConfig;
		node->configuration(nodeConfig);
		if(nodeConfig.pProperties)
		{
			PropertyID propID = 0;
			QJsonArray jsonProperties;

			while(nodeConfig.pProperties[propID].type != EPropertyType::Unknown)
			{
				NodeProperty propValue = node->property(propID);
				if(propValue.isValid())
					jsonProperties.append(serializeProperty(propID, nodeConfig.pProperties[propID], propValue));
				++propID;
			}

			if(!jsonProperties.isEmpty())
				jsonNode.insert(QStringLiteral("properties"), jsonProperties);
		}

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
										 const std::string& filePath)
{
	QString filePath_ = QString::fromStdString(filePath);
	QFile file(filePath_);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QByteArray fileContents = file.readAll();
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(fileContents, &error);
	if(error.error != QJsonParseError::NoError)
	{
		qWarning() << QString("Couldn't open node tree from file: %1\n"
			"Error details: %2 in %3 character")
			.arg(filePath_)
			.arg(error.errorString())
			.arg(error.offset);
		return false;
	}

	if(!doc.isObject())
	{
		qWarning() << QString("Couldn't open node tree from file: %1\n"
			"Error details: root value isn't JSON object")
			.arg(filePath_);
		return false;
	}

	QJsonObject jsonTree = doc.object();
	if(_rootDirectory.empty())
		_rootDirectory = QFileInfo(filePath_).absolutePath().toStdString();
	return deserializeJson(nodeTree, jsonTree, nullptr);
}

bool NodeTreeSerializer::deserializeJson(std::shared_ptr<NodeTree>& nodeTree,
										 const QJsonObject& jsonTree, 
										 std::map<NodeID, NodeID>* oldToNewNodeID)
{
	nodeTree->clear();
	std::map<NodeID, NodeID> mapping;

	if (!deserializeJsonNodes(nodeTree, jsonTree["nodes"].toArray(), mapping))
	{
		qCritical("Didn't load all saved nodes, tree will be discarded");
		nodeTree->clear();
		return false;
	}
	if (!deserializeJsonLinks(nodeTree, jsonTree["links"].toArray(), mapping))
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
					qWarning() << QString("property \"id\" is bad, property will be skipped (id:%1, name:%2)")
						.arg(nodeId)
						.arg(nodeName);
					continue;
				}

				QVariant qvalue = propMap["value"];
				if(!qvalue.isValid())
				{
					qWarning() << QString("\"value\" is bad, property of id %1 will be skipped (id:%2, name:%3)")
						.arg(propId)
						.arg(nodeId)
						.arg(nodeName);
					continue;
				}

				QString qpropType = propMap["type"].toString();
				EPropertyType propType = deserializePropertyType(qpropType.toStdString());
				if(propType == EPropertyType::Unknown)
				{
					qWarning() << QString("\"type\" is bad, property of id %1 will be skipped (id:%2, name:%3)")
						.arg(propId)
						.arg(nodeId)
						.arg(nodeName);
					continue;
				}

				try
				{
					if(!nodeTree->nodeSetProperty(_nodeId, propId, deserializeProperty(propType, qvalue)))
					{
						qWarning() << "Couldn't set loaded property" << propId << "to" << qvalue;
					}
				}
				catch(std::exception& ex)
				{
					qWarning() << ex.what();
				}
			}
		}

		++i;
	}

	return nodes.count() == i;
}

bool NodeTreeSerializer::deserializeJsonLinks(std::shared_ptr<NodeTree>& nodeTree,
											  const QJsonArray& jsonLinks,
											  const std::map<NodeID, NodeID>& mapping)
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

		if(nodeTree->linkNodes(SocketAddress(mapping.at(fromNode), fromSocket, true),
							   SocketAddress(mapping.at(toNode), toSocket, false)) != ELinkNodesResult::Ok)
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

QJsonObject NodeTreeSerializer::serializeProperty(PropertyID propID,
												  const PropertyConfig& propertyConfig,
												  const NodeProperty& propValue)
{
	QJsonObject jsonProp;
	jsonProp.insert(QStringLiteral("id"), propID);
	jsonProp.insert(QStringLiteral("name"), QString::fromStdString(propertyConfig.name));

	switch(propertyConfig.type)
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
		jsonProp.insert(QStringLiteral("value"), (int) propValue.toEnum().data());
		jsonProp.insert(QStringLiteral("type"), QStringLiteral("enum"));
		break;
	case EPropertyType::Matrix:
		{
			QJsonArray jsonMatrix;
			Matrix3x3 matrix = propValue.toMatrix3x3();
			for(double v : matrix.v)
				jsonMatrix.append(v);
			jsonProp.insert(QStringLiteral("value"), jsonMatrix);
			jsonProp.insert(QStringLiteral("type"), QStringLiteral("matrix3x3"));
			break;
		}
	case EPropertyType::Filepath:
		{
			QString rootDirStr = QString::fromStdString(_rootDirectory);
			QString filePath = QString::fromStdString(propValue.toFilepath().data());
			QDir rootDir(rootDirStr);
			if(rootDir.exists())
				jsonProp.insert(QStringLiteral("value"), rootDir.relativeFilePath(filePath));
			else
				jsonProp.insert(QStringLiteral("value"), filePath);
			jsonProp.insert(QStringLiteral("type"), QStringLiteral("filepath"));
		}
		break;
	case EPropertyType::String:
		jsonProp.insert(QStringLiteral("value"), QString::fromStdString(propValue.toString()));
		jsonProp.insert(QStringLiteral("type"), QStringLiteral("string"));
		break;
	case EPropertyType::Unknown:
		break;
	}

	return jsonProp;
}

EPropertyType NodeTreeSerializer::deserializePropertyType(const std::string& type)
{
	if(type == "boolean")
		return EPropertyType::Boolean;
	else if(type == "integer")
		return EPropertyType::Integer;
	else if(type == "double")
		return EPropertyType::Double;
	else if(type == "enum")
		return EPropertyType::Enum;
	else if(type == "matrix3x3")
		return EPropertyType::Matrix;
	else if(type == "filepath")
		return EPropertyType::Filepath;
	else if(type == "string")
		return EPropertyType::String;
	else
		return EPropertyType::Unknown;
}

NodeProperty NodeTreeSerializer::deserializeProperty(EPropertyType propType, 
													 const QVariant& qvalue)
{
	switch(propType)
	{
	case EPropertyType::Boolean:
		return NodeProperty(qvalue.toBool());
	case EPropertyType::Integer:
		return NodeProperty(qvalue.toInt());
	case EPropertyType::Double:
		return NodeProperty(qvalue.toDouble());
	case EPropertyType::Enum:
		return NodeProperty(Enum(qvalue.toUInt()));
	case EPropertyType::Matrix:
		{
			QVariantList matrixList = qvalue.toList();
			Matrix3x3 matrix;
			for(int i = 0; i < matrixList.count() && i < 9; ++i)
				matrix.v[i] = matrixList[i].toDouble();
			return NodeProperty(matrix);
		}
	case EPropertyType::Filepath:
		{
			QString rootDirStr = QString::fromStdString(_rootDirectory);
			QDir rootDir(rootDirStr);
			if(rootDir.exists())
				return NodeProperty(Filepath(rootDir.absoluteFilePath(qvalue.toString()).toStdString()));
			return NodeProperty();
		}
	case EPropertyType::String:
		return NodeProperty(qvalue.toString().toStdString());
	}

	return NodeProperty();
}