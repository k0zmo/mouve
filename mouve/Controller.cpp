#include "Controller.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "NodeLinkView.h"
#include "NodeSocketView.h"

/// xXx: This is needed here for now
#include "BuiltinNodeTypes.h"

#include "NodeSystem.h"
#include "NodeTree.h"
#include "NodeType.h"
#include "NodeLink.h"
#include "Node.h"

#include <QMessageBox>

static bool DEBUG_LINKS = false;

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

	connect(mNodeScene, SIGNAL(selectionChanged()),
		this, SLOT(nodeSceneSelectionChanged()));

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

	/// xXx: Only temporary solution
	connect(mExecute, SIGNAL(clicked()),
		this, SLOT(executeClicked()));
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
			(nullptr, "mouve", "[NodeTree] Couldn't create given node");
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
			(nullptr, "mouve", "[NodeTree] Error during querying node configuration");
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

	mNodeTree->tagNode(nodeID);
	mNodeTree->step();
}

void Controller::linkNodeViews(NodeSocketView* from, NodeSocketView* to)
{
	Q_ASSERT(from);
	Q_ASSERT(from->nodeView());
	Q_ASSERT(to);
	Q_ASSERT(to->nodeView());
	/// xXx: Add some checking to release

	SocketAddress addrFrom(from->nodeView()->nodeKey(), from->socketKey(), true);
	SocketAddress addrTo(to->nodeView()->nodeKey(), to->socketKey(), false);

	/// xXx: give a reason
	if(!mNodeTree->linkNodes(addrFrom, addrTo))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeTree] Couldn't linked given sockets");
		return;
	}

	// Create new link view 
	NodeLinkView* link = new NodeLinkView(from, to, nullptr);
	link->setDrawDebug(DEBUG_LINKS);
	from->addLink(link);
	to->addLink(link);

	// Add it to a scene
	mLinkViews.append(link);
	mNodeScene->addItem(link);

	// Tag and execute
	mNodeTree->tagNode(addrTo.node);
	mNodeTree->step();
	refreshSelectedNode(to->nodeView());
}

void Controller::unlinkNodeViews(NodeLinkView* linkView)
{
	Q_ASSERT(linkView);
	Q_ASSERT(linkView->fromSocketView());
	Q_ASSERT(linkView->toSocketView());
	/// xXx: Add some checking to release

	const NodeSocketView* from = linkView->fromSocketView();
	const NodeSocketView* to = linkView->toSocketView();

	SocketAddress addrFrom(from->nodeView()->nodeKey(),
	                       from->socketKey(), true);
	SocketAddress addrTo(to->nodeView()->nodeKey(), 
	                     to->socketKey(), false);

	/// xXx: give a reason
	if(!mNodeTree->unlinkNodes(addrFrom, addrTo))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeTree] Couldn't unlinked given sockets");
		return;
	}

	// Remove link view
	if(!mLinkViews.removeOne(linkView))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[Controller] Couldn't removed link view");
		return;
	}

	// Tag and execute
	mNodeTree->tagNode(addrTo.node);
	mNodeTree->step();

	/// xXx: doesn't work because we clear selection before
	refreshSelectedNode(to->nodeView());

	mNodeScene->removeItem(linkView);
	delete linkView;
}


void Controller::deleteNodeView(NodeView* nodeView)
{
	Q_ASSERT(nodeView);

	// Try to remove the model first
	NodeID nodeID = NodeID(nodeView->nodeKey());

	if(!mNodeTree->removeNode(nodeID))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeTree] Couldn't removed given node");
		return;
	}

	// Remove incoming and outgoing links
	// This should be done in cooperation with model but it would probably make the case harder
	/// xXx: Mark this as minor TODO
	///      We could retrieve the list of all removed links from NodeTree::removeNode 
	///      We would also need a method:
	///      NodeLinkView* resolveLinkView(const NodeLink& nodeLink);
	QMutableListIterator<NodeLinkView*> it(mLinkViews);
	while(it.hasNext())
	{
		NodeLinkView* linkView = it.next();

		if(linkView->inputConnecting(nodeView) ||
		   linkView->outputConnecting(nodeView))
		{
			// tag affected node
			Q_ASSERT(linkView->toSocketView());
			Q_ASSERT(linkView->toSocketView()->nodeView());
			mNodeTree->tagNode(linkView->toSocketView()->nodeView()->nodeKey());

			// remove link view
			mNodeScene->removeItem(linkView);
			it.remove();
			delete linkView;
		}
	}

	// Remove node view from the scene
	mNodeScene->removeItem(nodeView);
	// Remove from the container
	mNodeViews.remove(nodeID);
	// Finally remove the node view itself
	nodeView->deleteLater();

	mNodeTree->step();
}

void Controller::refreshSelectedNode(NodeView* nodeView)
{
	QList<QGraphicsItem*> selectedItems = mNodeScene->selectedItems();
	if(selectedItems.count() == 1 && 
		selectedItems[0] == nodeView)
	{
		nodeSceneSelectionChanged();
	}
}

void Controller::draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to)
{
	linkNodeViews(static_cast<NodeSocketView*>(from),
	              static_cast<NodeSocketView*>(to));
}

void Controller::draggingLinkStarted(QGraphicsWidget* from)
{
	mNodeScene->setDragging(true);
}

void Controller::draggingLinkStopped(QGraphicsWidget* from)
{
	mNodeScene->setDragging(false);
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
	else
	{
		for(int i = 0; i < items.size(); ++i)
		{
			auto item = items[i];

			if(item->type() == NodeView::Type)
			{
				QAction action("Remove node", nullptr);
				QMenu menu;
				menu.addAction(&action);
				QAction* ret = menu.exec(globalPos);
				if(ret != nullptr)
				{
					NodeView* nodeView = static_cast<NodeView*>(item);
					deleteNodeView(nodeView);
				}
				return;
			}
		}
	}
}

void Controller::keyPress(QKeyEvent* event)
{
	if(event->key() == Qt::Key_Delete)
	{
		QList<QGraphicsItem*> selectedItems = mNodeScene->selectedItems();
		QList<QGraphicsItem*> selectedNodeViews;
		mNodeScene->clearSelection();

		for(int i = 0; i < selectedItems.size(); ++i)
		{
			auto item = selectedItems[i];

			if(item->type() == NodeLinkView::Type)
			{
				NodeLinkView* linkView = static_cast<NodeLinkView*>(item);
				unlinkNodeViews(linkView);
			}
			else
			{
				selectedNodeViews.append(item);
			}
		}

		for(int i = 0; i < selectedNodeViews.size(); ++i)
		{
			auto item = selectedNodeViews[i];

			if(item->type() == NodeView::Type)
			{
				NodeView* nodeView = static_cast<NodeView*>(item);
				deleteNodeView(nodeView);
			}
		}

		event->accept();
	}
}

void Controller::executeClicked()
{
	// Tag everything - only for debugging
	auto ni = mNodeTree->createNodeIterator();
	NodeID nodeID;
	while(ni->next(nodeID))
		mNodeTree->tagNode(nodeID);

	mNodeTree->step();
	nodeSceneSelectionChanged();
}

void Controller::nodeSceneSelectionChanged()
{
	/// xXx: Temporary solution
	QList<QGraphicsItem*> selectedItems = mNodeScene->selectedItems();
	if(selectedItems.count() == 1)
	{
		QGraphicsItem* selectedItem = selectedItems[0];
		if(selectedItem->type() == NodeView::Type)
		{
			NodeView* nodeView = static_cast<NodeView*>(selectedItem);
			const cv::Mat& mat = mNodeTree->outputSocket(nodeView->nodeKey(), 0);

			QImage image = QImage(
				reinterpret_cast<const quint8*>(mat.data),
				mat.cols, mat.rows, mat.step, 
				QImage::Format_Indexed8);
			mOutputPreview->setPixmap(QPixmap::fromImage(image));
		}
	}
}