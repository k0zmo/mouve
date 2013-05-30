#include "NodeTree.h"
#include "Node.h"
#include "NodeType.h"
#include "NodeLink.h"
#include "NodeSystem.h"
#include "NodeModule.h"
#include "NodeException.h"

#include "Kommon/StlUtils.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
/// TODO: Add logger
#include <QDebug>

NodeTree::NodeTree(NodeSystem* nodeSystem)
	: _nodeSystem(nodeSystem)
	, _executeListDirty(false)
{
}

NodeTree::~NodeTree()
{
	clear();
}

void NodeTree::clear()
{
	_nodes.clear();
	_recycledIDs.clear();
	_links.clear();
	_executeList.clear();
	_nodeNameToNodeID.clear();
	_executeListDirty = false;
}

QJsonObject NodeTree::serializeJson()
{
	// Iterate over all nodes and serialize it as JSON value of JSON array
	QJsonArray jsonNodes;
	auto nodeIt = createNodeIterator();
	NodeID nodeID;
	while(const Node* node = nodeIt->next(nodeID))
	{
		QJsonObject jsonNode;

		QString typeName = QString::fromStdString(nodeTypeName(nodeID));
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
					auto&& prop = nodeConfig.pProperties[propID];

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
						jsonProp.insert(QStringLiteral("value"), propValue.toString());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("filepath"));
						break;
					case EPropertyType::String:
						jsonProp.insert(QStringLiteral("value"), propValue.toString());
						jsonProp.insert(QStringLiteral("type"), QStringLiteral("string"));
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
	auto linkIt = createNodeLinkIterator();
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

bool NodeTree::deserializeJson(const QJsonObject& jsonTree, 
	std::map<NodeID, NodeID>* oldToNewNodeID)
{
	clear();

	QJsonArray nodes = jsonTree["nodes"].toArray();
	QJsonArray links = jsonTree["links"].toArray();

	/// TODO : check if the "nodes", "links" are indeed QVariantList?
	std::map<NodeID, NodeID> mapping;

	if(!deserializeJsonNodes(nodes, mapping))
	{
		qCritical("Didn't load all saved nodes, tree will be discarded");
		clear();
		return false;
	}
	if(!deserializeJsonLinks(links, mapping))
	{
		qCritical("Didn't load all saved links, tree will be discarded");
		clear();
		return false;
	}

	if(oldToNewNodeID != nullptr)
		*oldToNewNodeID = mapping;

	return true;
}

bool NodeTree::deserializeJsonNodes(const QJsonArray& jsonNodes,
									std::map<NodeID, NodeID>& mapping)
{
	QVariantList nodes = jsonNodes.toVariantList();
	int i = 0;

	// Load nodes
	for(auto&& node : nodes)
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
		NodeTypeID nodeTypeId = _nodeSystem->nodeTypeID(nodeTypeName.toStdString());
		NodeID _nodeId = createNode(nodeTypeId, nodeName.toStdString());
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

			for(auto&& prop : properties)
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

				if(!nodeSetProperty(_nodeId, propId, value))
				{
					qWarning() << "Couldn't set loaded property" << propId <<  "to" << value;
				}
			}
		}

		++i;
	}

	return nodes.count() == i;
}

bool NodeTree::deserializeJsonLinks(const QJsonArray& jsonLinks,
									std::map<NodeID, NodeID>& mapping)
{
	int i = 0;
	QVariantList links = jsonLinks.toVariantList();

	for(auto&& link : links)
	{
		QVariantMap mapLink = link.toMap();

		NodeID fromNode = mapLink["fromNode"].toUInt();
		SocketID fromSocket = mapLink["fromSocket"].toUInt();
		NodeID toNode = mapLink["toNode"].toUInt();
		SocketID toSocket = mapLink["toSocket"].toUInt();

		if(!linkNodes(SocketAddress(mapping[fromNode], fromSocket, true),
			SocketAddress(mapping[toNode], toSocket, false)))
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

size_t NodeTree::nodesCount() const
{
	return _nodes.size();
}

void NodeTree::tagNode(NodeID nodeID)
{
	if(!validateNode(nodeID))
		return;

	_nodes[nodeID].setFlag(ENodeFlags::Tagged);
	_executeListDirty = true;
}

void NodeTree::untagNode(NodeID nodeID)
{
	if(!validateNode(nodeID))
		return;

	auto&& node = _nodes[nodeID];

	if(node.flag(ENodeFlags::Tagged))
	{
		node.unsetFlag(ENodeFlags::Tagged);
		_executeListDirty = true;
	}
}

void NodeTree::notifyFinish()
{
	for(NodeID nodeID = 0; nodeID < NodeID(_nodes.size()); ++nodeID)
	{
		if(!validateNode(nodeID))
			continue;

		_nodes[nodeID].finish();

		// Finally, tag node that need to be auto-tagged
		if(_nodes[nodeID].flag(ENodeFlags::AutoTag))
			tagNode(nodeID);
	}
}

bool NodeTree::isTreeStateless() const
{
	for(const Node& node : _nodes)
	{
		if(node.flag(ENodeFlags::StateNode))
			return false;
	}

	return true;
}

std::vector<NodeID> NodeTree::prepareList()
{
	if(!_executeListDirty)
		return _executeList;
	_executeList.clear();

	// stable_sort doesn't touch already sorted items (?)
	//std::sort(std::begin(links), std::end(links));
	std::stable_sort(std::begin(_links), std::end(_links));

	for(NodeID nodeID = 0; nodeID < NodeID(_nodes.size()); ++nodeID)
	{
		if(!_nodes[nodeID].flag(ENodeFlags::Tagged))
			continue;

		if(!validateNode(nodeID))
			continue;

		if(!allRequiredInputSocketConnected(nodeID))
			continue;

		if(!stlu::contains_value(_executeList, nodeID))
		{
			addToExecuteList(_executeList, nodeID);
			traverseRecurs(_executeList, nodeID);
		}
	}

	_executeListDirty = false;
	return _executeList;
}

void NodeTree::cleanUpAfterExecution(const std::vector<NodeID>& selfTagging,
									 const std::vector<NodeID>& correctlyExecutedNodes)
{
	// Only untag nodes that were correctly executed
	for(NodeID id : correctlyExecutedNodes)
		_nodes[id].unsetFlag(ENodeFlags::Tagged);
	_executeListDirty = true;

	// tag all self-tagging nodes
	for(NodeID id : selfTagging)
		tagNode(id);
}

void NodeTree::handleException(const std::string& nodeName, const std::string& nodeTypeName)
{
	// Exception dispatcher pattern
	try
	{
		throw;
	}
	catch(ExecutionError&)
	{
		// Dummy but a must - if not std::exception handler would catch it
		throw;
	}
	catch(boost::bad_get&)
	{
		throw ExecutionError(nodeName, nodeTypeName, 
			"Wrong socket connection");
	}
	catch(cv::Exception& ex)
	{
		throw ExecutionError(nodeName, nodeTypeName, 
			std::string("OpenCV exception - ") + ex.what());
	}
	catch(std::exception& ex)
	{
		throw ExecutionError(nodeName, nodeTypeName, ex.what());
	}
}

void NodeTree::execute(bool withInit)
{
	if(_executeListDirty)
		prepareList();

	NodeSocketReader reader(this);
	NodeSocketWriter writer;

	std::vector<NodeID> selfTagging;
	std::vector<NodeID> correctlyExecutedNodes;

	// Traverse through just-built exec list and process each node 
	for(NodeID nodeID : _executeList)
	{
		Node& node = _nodes[nodeID];

		reader.setNode(nodeID, node.numInputSockets());

		try
		{
			if(withInit && _nodes[nodeID].flag(ENodeFlags::StateNode))
			{
				// Try to restart node internal state if any
				if(!node.restart())
				{
					throw ExecutionError(node.nodeName(), 
						nodeTypeName(nodeID), "Error during node state restart");
				}
			}

			ExecutionStatus ret = node.execute(reader, writer);
			if(ret.status == EStatus::Tag)
			{
				selfTagging.push_back(nodeID);
			}
			else if(ret.status == EStatus::Error)
			{   // If node reported an error
				selfTagging.push_back(nodeID);
				throw ExecutionError(node.nodeName(), 
					nodeTypeName(nodeID), ret.message);
			}

			correctlyExecutedNodes.push_back(nodeID);
		}
		catch(...)
		{
			cleanUpAfterExecution(selfTagging, correctlyExecutedNodes);
			handleException(node.nodeName(), nodeTypeName(nodeID));
		}
	}

	cleanUpAfterExecution(selfTagging, correctlyExecutedNodes);
}

std::vector<NodeID> NodeTree::executeList() const
{
	return _executeList;
}

NodeID NodeTree::createNode(NodeTypeID typeID, const std::string& name)
{
	if(!_nodeSystem)
		return InvalidNodeID;

	// Check if the given name is unique
	if(_nodeNameToNodeID.find(name) != _nodeNameToNodeID.end())
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

	// If node type belongs to a registered module
	NodeConfig nodeConfig;
	nodeType->configuration(nodeConfig);
	if(!nodeConfig.module.empty())
	{
		bool res = false;
		auto&& module = _nodeSystem->nodeModule(nodeConfig.module);
		if(module && module->ensureInitialized())
			res = nodeType->init(module);

		// Module not initialized - rollback
		if(!res)
		{
			deallocateNodeID(id);
			return InvalidNodeID;
		}
	}

	// Create actual node object
	_nodes[id] = Node(std::move(nodeType), name, typeID);
	// Add the pair <node name, node ID> to hash map
	_nodeNameToNodeID.insert(std::make_pair(name, id));
	// Tag it for next execution
	tagNode(id);

	return id;
}

bool NodeTree::removeNode(NodeID nodeID)
{
	if(!validateNode(nodeID))
		return false;

	// Tag nodes that are directly connected to the removed node
	for(auto&& nodeLink : _links)
	{
		if(nodeLink.fromNode == nodeID)
			tagNode(nodeLink.toNode);
	}

	// Remove any referring links
	auto removeFirstNodeLink = std::remove_if(
		std::begin(_links), std::end(_links),
		[&](const NodeLink& nodeLink)
		{
			return nodeLink.fromNode == nodeID ||
			       nodeLink.toNode   == nodeID;
		});
	_links.erase(removeFirstNodeLink, std::end(_links));

	// Finally remove the node
	deallocateNodeID(nodeID);

	return true;
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

	// Check if we are not trying to link input socket with more than one output
	for(size_t i = 0; i < _links.size(); ++i)
	{
		auto&& link = _links[i];

		if(link.toNode == to.node &&
			link.toSocket == to.socket)
		{
			return false;
		}
	}

	_links.emplace_back(NodeLink(from, to));
	// tag affected node
	tagNode(to.node);

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
		// Need to store ID before erase() invalidates iterator
		NodeID nodeID = iter->toNode;

		_links.erase(iter);

		// tag affected node
		tagNode(nodeID);

		return true;
	}

	return false;
}

void NodeTree::setNodeName(NodeID nodeID, const std::string& newNodeName)
{
	if(nodeID < _nodes.size())
	{
		auto&& node = _nodes[nodeID];
		if(node.isValid() && node.nodeName() != newNodeName)
		{
			// Remove old mapping
			_nodeNameToNodeID.erase(node.nodeName());

			// Change a name and add a new mapping
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
		auto&& node = _nodes[nodeID];
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
	// This function does not rely on a assumption that links are sorted
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

const NodeFlowData& NodeTree::outputSocket(NodeID nodeID, SocketID socketID) const
{
	if(!validateNode(nodeID))
		throw BadNodeException();

	if(!allRequiredInputSocketConnected(nodeID))
	{
		/// TODO: Transform this to a method (same as NodeType::readSocket())
		static NodeFlowData _default = NodeFlowData();
		return _default;
	}

	// outputSocket verifies socketID
	return _nodes[nodeID].outputSocket(socketID);
}

const NodeFlowData& NodeTree::inputSocket(NodeID nodeID, SocketID socketID) const
{
	if(!validateNode(nodeID))
		throw BadNodeException();

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

bool NodeTree::allRequiredInputSocketConnected(NodeID nodeID) const
{
	if(!validateNode(nodeID))
		return false;

	auto&& node = _nodes[nodeID];

	for(SocketID socketID = 0; socketID < node.numInputSockets(); ++socketID)
	{
		SocketAddress outputAddr = connectedFrom(SocketAddress(nodeID, socketID, false));
		if(outputAddr.node == InvalidNodeID || outputAddr.socket == InvalidSocketID)
			return false;
	}

	return true;
}

bool NodeTree::taggedButNotExecuted(NodeID nodeID) const
{
	// Node is tagged ..
	if(_nodes[nodeID].flag(ENodeFlags::Tagged)
		// .. and isn't on the executed list
		&& std::find(_executeList.begin(), _executeList.end(), nodeID) == _executeList.end())
	{
		return true;
	}

	return false;
}

bool NodeTree::nodeConfiguration(NodeID nodeID, NodeConfig& nodeConfig) const
{
	if(!validateNode(nodeID))
		return false;

	_nodes[nodeID].configuration(nodeConfig);
	return true;
}

bool NodeTree::nodeSetProperty(NodeID nodeID, PropertyID propID, const QVariant& value)
{
	if(!validateNode(nodeID))
		return false;

	return _nodes[nodeID].setProperty(propID, value);
}

QVariant NodeTree::nodeProperty(NodeID nodeID, PropertyID propID)
{
	if(!validateNode(nodeID))
		return QVariant();

	return _nodes[nodeID].property(propID);
}

const std::string& NodeTree::nodeExecuteInformation(NodeID nodeID) const
{
	static const std::string empty;
	if(!validateNode(nodeID))
		return empty;

	return _nodes[nodeID].executeInformation();
}

double NodeTree::nodeTimeElapsed(NodeID nodeID) const
{
	if(!validateNode(nodeID))
		return 0.0;

	return _nodes[nodeID].timeElapsed();
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

	untagNode(id);

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
		auto&& link = _links[i];

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
		size_t firstOutputLinkIndex = firstOutputLink(nodeID, socketID, lastSocketID);
		if(firstOutputLinkIndex == size_t(-1))
			continue;
		lastSocketID = firstOutputLinkIndex;

		auto iter    = std::begin(_links) + firstOutputLinkIndex;
		auto iterEnd = std::end(_links);

		while(iter != iterEnd &&
			  iter->fromNode == nodeID &&
			  iter->fromSocket == socketID)
		{
			NodeID nodeID = iter->toNode;

			if(allRequiredInputSocketConnected(nodeID))
			{
				addToExecuteList(execList, nodeID);
				traverseRecurs(execList, nodeID);
			}
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
