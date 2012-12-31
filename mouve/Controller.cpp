#include "Controller.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "NodeLinkView.h"

/// xXx: This is needed here for now
#include "BuiltinNodeTypes.h"

#include "NodeSystem.h"
#include "NodeTree.h"
#include "NodeType.h"

#include <QMessageBox>

template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
	, mNodeScene(new NodeScene(this))
	, mNodeSystem(new NodeSystem())
{
	setupUi(this);

	// Set up a node scene
	/// xXx: Temporary
	mNodeScene->setSceneRect(-200,-200,1000,600);
	// Qt bug concering scene->removeItem ?? Seems to fixed it
	mNodeScene->setItemIndexMethod(QGraphicsScene::NoIndex);
	mGraphicsView->setScene(mNodeScene);

	// Context menu from node graphics view
	connect(mGraphicsView, SIGNAL(contextMenu(QPoint,QPointF)),
		this, SLOT(contextMenu(QPoint,QPointF)));
	// Key handler from node graphics view
	connect(mGraphicsView, SIGNAL(keyPress(QKeyEvent*)),
		this, SLOT(keyPress(QKeyEvent*)));

	/// xXx: Create our node tree
	mNodeTree = mNodeSystem->createNodeTree();

	// Get available nodes and and them to Add Node context menu
	auto nodeTypeIterator = mNodeSystem->createNodeTypeIterator();
	NodeTypeIterator::NodeTypeInfo info;
	while(nodeTypeIterator->next(info))
	{
		QAction* action = new QAction(
			QString::fromStdString(info.typeName), this);
		action->setData(info.typeID);
		mAddNodesActions.append(action);
	}
}

Controller::~Controller()
{
	foreach(NodeLinkView* link, mLinkViews)
		delete link;
	mLinkViews.clear();

	foreach(NodeView* node, mNodeViews.values())
		delete node;
	mNodeViews.clear();
}

void Controller::addNode(NodeTypeID nodeTypeID, const QPointF& scenePos)
{
	// Create new model
	/// xXx: Get a title from a class name or a default name field
	std::string defaultNodeTitle = mNodeSystem->nodeTypeName(nodeTypeID);

	// Generate unique name
	std::string nodeTitle = defaultNodeTitle;
	int num = 1;
	while(mNodeTree->resolveNode(nodeTitle) != InvalidNodeID)
	{
		QString newName = QString(QString::fromStdString(defaultNodeTitle) + 
			" " + QString::number(num));
		nodeTitle = newName.toStdString();
		++num;
	}

	// Try to create new node 
	NodeID nodeID = mNodeTree->createNode(nodeTypeID, nodeTitle);
	if(nodeID == InvalidNodeID)
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeSystem] Couldn't create given node");
		return;
	}

	// Create new view associated with the model
	addNodeView(QString::fromStdString(nodeTitle), nodeID, scenePos);
}

void Controller::addNodeView(const QString& nodeTitle,
	NodeID nodeID, const QPointF& scenePos)
{
	NodeView* nodeView = new NodeView(nodeTitle);
	nodeView->setData(NodeDataIndex::NodeKey, nodeID);
	nodeView->setPos(scenePos);

	NodeConfig nodeConfig;
	if(!mNodeTree->nodeConfiguration(nodeID, nodeConfig))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeSystem] Error during querying node configuration");
		return;
	}

	// Add input sockets views to node view
	SocketID socketID = 0;
	if(nodeConfig.pInputSockets)
	{
		while(nodeConfig.pInputSockets[socketID].name.length() > 0)
		{
			auto& name = nodeConfig.pInputSockets[socketID].name;
			auto& humanName = nodeConfig.pInputSockets[socketID].humanName;

			QString socketTitle = humanName.length() > 0
				? QString::fromStdString(humanName)
				: QString::fromStdString(name);
			nodeView->addSocketView(socketID, socketTitle, false);
			++socketID;
		}
	}

	socketID = 0;
	// Add input sockets views 
	if(nodeConfig.pOutputSockets)
	{
		while(nodeConfig.pOutputSockets[socketID].name.length() > 0)
		{
			auto& name = nodeConfig.pOutputSockets[socketID].name;
			auto& humanName = nodeConfig.pOutputSockets[socketID].humanName;

			QString socketTitle = humanName.length() > 0
				? QString::fromStdString(humanName)
				: QString::fromStdString(name);
			nodeView->addSocketView(socketID, socketTitle, true);
			++socketID;
		}
	}

	mNodeViews[nodeID] = nodeView;
	mNodeScene->addItem(nodeView);
}

void Controller::draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to)
{

}

void Controller::draggingLinkStarted(QGraphicsWidget* from)
{

}

void Controller::draggingLinkStopped(QGraphicsWidget* from)
{

}

void Controller::contextMenu(const QPoint& globalPos,
	const QPointF& scenePos)
{
	QList<QGraphicsItem*> items = mNodeScene->items(scenePos, 
		Qt::ContainsItemShape, Qt::AscendingOrder);

	//If we clicked onto empty space
	if(items.isEmpty())
	{
		QMenu menu;
		QMenu* menuAddNode = menu.addMenu("Add node");
		foreach(QAction* a, mAddNodesActions)
			menuAddNode->addAction(a);
		QAction* ret = menu.exec(globalPos);
		if(ret != nullptr)
			addNode(ret->data().toInt(), scenePos);
	}
}

void Controller::keyPress(QKeyEvent* event)
{
}