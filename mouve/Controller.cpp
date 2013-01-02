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

#include "ui_MainWindow.h"

static bool DEBUG_LINKS = false;

template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
	, _previewSelectedNodeView(nullptr)
	, _nodeScene(new NodeScene(this))
	, _nodeSystem(new NodeSystem())
	, _ui(new Ui::MainWindow())
{
	_ui->setupUi(this);

	// Set up a node scene
	/// xXx: Temporary
	_nodeScene->setSceneRect(-200,-200,1000,600);
	// Qt bug concering scene->removeItem ?? Seems to fixed it
	_nodeScene->setItemIndexMethod(QGraphicsScene::NoIndex);

	_ui->graphicsView->setScene(_nodeScene);

	// Context menu from node graphics view
	connect(_ui->graphicsView, SIGNAL(contextMenu(QPoint,QPointF)),
		this, SLOT(contextMenu(QPoint,QPointF)));
	// Key handler from node graphics view
	connect(_ui->graphicsView, SIGNAL(keyPress(QKeyEvent*)),
		this, SLOT(keyPress(QKeyEvent*)));

	/// xXx: Create our node tree
	_nodeTree = _nodeSystem->createNodeTree();

	// Get available nodes and and them to Add Node context menu
	auto nodeTypeIterator = _nodeSystem->createNodeTypeIterator();
	NodeTypeIterator::NodeTypeInfo info;
	while(nodeTypeIterator->next(info))
	{
		QAction* action = new QAction(
			QString::fromStdString(info.typeName), this);
		action->setData(info.typeID);
		_addNodesActions.append(action);
	}

	/// xXx: Only temporary solution
	connect(_ui->execute, SIGNAL(clicked()),
		this, SLOT(executeClicked()));
}

Controller::~Controller()
{
	foreach(NodeLinkView* link, _linkViews)
		delete link;
	_linkViews.clear();

	foreach(NodeView* node, _nodeViews.values())
		delete node;
	_nodeViews.clear();

	delete _ui;
}

void Controller::addNode(NodeTypeID nodeTypeID, const QPointF& scenePos)
{
	// Create new model
	/// xXx: Get a title from a class name or a default name field
	std::string defaultNodeTitle = _nodeSystem->nodeTypeName(nodeTypeID);

	// Generate unique name
	std::string nodeTitle = defaultNodeTitle;
	int num = 1;
	while(_nodeTree->resolveNode(nodeTitle) != InvalidNodeID)
	{
		QString newName = QString(QString::fromStdString(defaultNodeTitle) + 
			" " + QString::number(num));
		nodeTitle = newName.toStdString();
		++num;
	}

	// Try to create new node 
	NodeID nodeID = _nodeTree->createNode(nodeTypeID, nodeTitle);
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
	if(!_nodeTree->nodeConfiguration(nodeID, nodeConfig))
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

	connect(nodeView, SIGNAL(mouseDoubleClicked(NodeView*)),
		this, SLOT(mouseDoubleClickNodeView(NodeView*)));

	_nodeViews[nodeID] = nodeView;
	_nodeScene->addItem(nodeView);

	_nodeTree->tagNode(nodeID);
	_nodeTree->step();
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
	if(!_nodeTree->linkNodes(addrFrom, addrTo))
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
	_linkViews.append(link);
	_nodeScene->addItem(link);

	// Tag and execute
	_nodeTree->tagNode(addrTo.node);
	_nodeTree->step();

	if(_previewSelectedNodeView == to->nodeView())
		updatePreview();
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
	if(!_nodeTree->unlinkNodes(addrFrom, addrTo))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[NodeTree] Couldn't unlinked given sockets");
		return;
	}

	// Remove link view
	if(!_linkViews.removeOne(linkView))
	{
		QMessageBox::critical
			(nullptr, "mouve", "[Controller] Couldn't removed link view");
		return;
	}

	// Tag and execute
	_nodeTree->tagNode(addrTo.node);
	_nodeTree->step();

	if(to->nodeView() && _previewSelectedNodeView == to->nodeView())
		updatePreview();

	_nodeScene->removeItem(linkView);
	delete linkView;
}


void Controller::deleteNodeView(NodeView* nodeView)
{
	Q_ASSERT(nodeView);

	// Try to remove the model first
	NodeID nodeID = NodeID(nodeView->nodeKey());

	if(!_nodeTree->removeNode(nodeID))
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
	bool needPreviewUpdate = false;
	QMutableListIterator<NodeLinkView*> it(_linkViews);
	while(it.hasNext())
	{
		NodeLinkView* linkView = it.next();

		if(linkView->inputConnecting(nodeView) ||
		   linkView->outputConnecting(nodeView))
		{
			auto sv = linkView->toSocketView();
			Q_ASSERT(sv);
			Q_ASSERT(sv->nodeView());

			// tag affected node
			_nodeTree->tagNode(sv->nodeView()->nodeKey());

			// Check if we need an update on preview window
			if(!needPreviewUpdate && 
			   sv->nodeView() == _previewSelectedNodeView)
			{
				needPreviewUpdate = true;
			}

			// remove link view
			_nodeScene->removeItem(linkView);
			it.remove();
			delete linkView;
		}
	}

	// Remove node view from the scene
	_nodeScene->removeItem(nodeView);
	// Remove from the container
	_nodeViews.remove(nodeID);

	_nodeTree->step();

	// update preview window
	if(nodeView == _previewSelectedNodeView)
	{
		_previewSelectedNodeView = nullptr;
		updatePreview();
	}
	else if(needPreviewUpdate)
	{
		updatePreview();
	}

	// Finally remove the node view itself
	nodeView->deleteLater();
}

void Controller::draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to)
{
	linkNodeViews(static_cast<NodeSocketView*>(from),
	              static_cast<NodeSocketView*>(to));
}

void Controller::draggingLinkStart(QGraphicsWidget* from)
{
	_nodeScene->setDragging(true);
}

void Controller::draggingLinkStop(QGraphicsWidget* from)
{
	_nodeScene->setDragging(false);
}

void Controller::contextMenu(const QPoint& globalPos,
	const QPointF& scenePos)
{
	QList<QGraphicsItem*> items = _nodeScene->items(scenePos, 
		Qt::ContainsItemShape, Qt::AscendingOrder);

	//If we clicked onto empty space
	if(items.isEmpty())
	{
		QMenu menu;
		QMenu* menuAddNode = menu.addMenu("Add node");
		foreach(QAction* a, _addNodesActions)
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
		QList<QGraphicsItem*> selectedItems = _nodeScene->selectedItems();
		QList<QGraphicsItem*> selectedNodeViews;
		_nodeScene->clearSelection();

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
	auto ni = _nodeTree->createNodeIterator();
	NodeID nodeID;
	while(ni->next(nodeID))
		_nodeTree->tagNode(nodeID);

	_nodeTree->step();
	updatePreview();
}

void Controller::mouseDoubleClickNodeView(NodeView* nodeView)
{
	if(_previewSelectedNodeView != nodeView)
	{
		if(_previewSelectedNodeView != nullptr)
			_previewSelectedNodeView->selectPreview(false);

		_previewSelectedNodeView = nodeView;

		if(_previewSelectedNodeView != nullptr)
		{
			_previewSelectedNodeView->selectPreview(true);
			updatePreview();
		}
	}
}

void Controller::updatePreview()
{
	if(_previewSelectedNodeView != nullptr)
	{
		/// xXx: For now we preview only first output socket
		///      This shouldn't be much a problem
		const cv::Mat& mat = _nodeTree->outputSocket(_previewSelectedNodeView->nodeKey(), 0);
		cv::Mat mat_;
			
		// Scale it up nicely
		/// xXx: In future we should use OpenGL and got free scaling
		static const int maxImageWidth = 256;
		static const int maxImageHeight = 256;

		if(mat.rows > maxImageHeight || 
		   mat.cols > maxImageWidth)
		{
			double fx;
			if(mat.rows > mat.cols)
				fx = static_cast<double>(maxImageHeight) / mat.rows;
			else
				fx = static_cast<double>(maxImageWidth) / mat.cols;
			cv::resize(mat, mat_, cv::Size(0,0), fx, fx, cv::INTER_LINEAR);
		}
		else
		{
			mat_ = mat;
		}

		QImage image = QImage(
			reinterpret_cast<const quint8*>(mat_.data),
			mat_.cols, mat_.rows, mat_.step, 
			QImage::Format_Indexed8);
		_ui->outputPreview->setPixmap(QPixmap::fromImage(image));
	}
	else
	{
		_ui->outputPreview->setPixmap(QPixmap());
	}
}