#include "Controller.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "NodeLinkView.h"
#include "NodeSocketView.h"

#include "PropertyDelegate.h"
#include "PropertyManager.h"
#include "PropertyModel.h"

/// xXx: This is needed here for now
#include "Logic/BuiltinNodeTypes.h"

#include "Logic/NodeSystem.h"
#include "Logic/NodeTree.h"
#include "Logic/NodeType.h"
#include "Logic/NodeLink.h"
#include "Logic/Node.h"

#include <QMessageBox>
#include <QMenu>

#include "ui_MainWindow.h"

static bool DEBUG_LINKS = false;
static const int maxImageWidth = 256;
static const int maxImageHeight = 256;

template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
	, _previewSelectedNodeView(nullptr)
	, _nodeScene(new NodeScene(this))
	, _nodeSystem(new NodeSystem())
	, _propManager(new PropertyManager(this))
	, _ui(new Ui::MainWindow())
{
	_ui->setupUi(this);

	/// TODO: use OpenGL
	_ui->outputPreview->setMinimumSize(maxImageWidth, maxImageHeight);
	_ui->outputPreview->setMaximumSize(maxImageWidth, maxImageHeight);

	_ui->actionQuit->setShortcuts(QKeySequence::Quit);
	connect(_ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);

	/// xXx: Only temporary, debugging solution
	connect(_ui->actionExecute, &QAction::triggered, this, &Controller::executeClicked);

	QAction* actionProperties = _ui->propertiesDockWidget->toggleViewAction();
	actionProperties->setShortcut(tr("Ctrl+1"));

	QAction* actionPreview = _ui->previewDockWidget->toggleViewAction();
	actionPreview->setShortcut(tr("Ctrl+2"));

	/// setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	/// tabifyDockWidget(_ui->propertiesDockWidget, _ui->previewDockWidget);

	// Init menu bar and its 'view' menu
	_ui->menuView->addAction(actionProperties);
	_ui->menuView->addAction(actionPreview);

	// Init properties window
	_ui->propertiesTreeView->setItemDelegateForColumn(1, 
		new PropertyDelegate(this));
	_ui->propertiesTreeView->header()->setSectionResizeMode(
		QHeaderView::ResizeToContents);

	// Set up a node scene
	/// xXx: Temporary
	///_nodeScene->setSceneRect(-200,-200,1000,600);

	connect(_nodeScene, &QGraphicsScene::selectionChanged,
		this, &Controller::sceneSelectionChanged);

	// Qt bug concering scene->removeItem ?? Seems to fixed it
	_nodeScene->setItemIndexMethod(QGraphicsScene::NoIndex);

	_ui->graphicsView->setScene(_nodeScene);

	// Context menu from node graphics view
	connect(_ui->graphicsView, &NodeEditorView::contextMenu,
		this, &Controller::contextMenu);
	// Key handler from node graphics view
	connect(_ui->graphicsView, &NodeEditorView::keyPress,
		this, &Controller::keyPress);

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
	std::string defaultNodeTitle = _nodeSystem->nodeTypeName(nodeTypeID);

	// Generate unique name
	std::string nodeTitle = defaultNodeTitle;
	int num = 1;
	while(_nodeTree->resolveNode(nodeTitle) != InvalidNodeID)
	{
		QString newName = QString("%1 %2")
			.arg(QString::fromStdString(defaultNodeTitle))
			.arg(num);
		nodeTitle = newName.toStdString();
		++num;
	}

	// Try to create new node 
	NodeID nodeID = _nodeTree->createNode(nodeTypeID, nodeTitle);
	if(nodeID == InvalidNodeID)
	{
		showErrorMessage("[NodeTree] Couldn't create given node");
		return;
	}

	// Create new view associated with the model
	addNodeView(QString::fromStdString(nodeTitle), nodeID, scenePos);

	// Tag and execute
	_nodeTree->tagNode(nodeID);
	std::vector<NodeID> execList = _nodeTree->step();

	updatePreview(execList);
}

void Controller::addNodeView(const QString& nodeTitle,
	NodeID nodeID, const QPointF& scenePos)
{
	NodeView* nodeView = new NodeView(nodeTitle);
	nodeView->setData(NodeDataIndex::NodeKey, nodeID);
	nodeView->setPos(scenePos);

	NodeConfig nodeConfig = {0};
	if(!_nodeTree->nodeConfiguration(nodeID, nodeConfig))
	{
		showErrorMessage("[NodeTree] Error during querying node configuration");
		return;
	}

	// Add input sockets views to node view
	SocketID socketID = 0;
	if(nodeConfig.pInputSockets)
	{
		while(nodeConfig.pInputSockets[socketID].name.length() > 0)
		{
			auto& input = nodeConfig.pInputSockets[socketID];

			QString socketTitle = input.humanName.length() > 0
				? QString::fromStdString(input.humanName)
				: QString::fromStdString(input.name);
			nodeView->addSocketView(socketID, socketTitle, false);
			++socketID;
		}
	}

	// Add output sockets views to node view
	socketID = 0;
	if(nodeConfig.pOutputSockets)
	{
		while(nodeConfig.pOutputSockets[socketID].name.length() > 0)
		{
			auto& output = nodeConfig.pOutputSockets[socketID];

			QString socketTitle = output.humanName.length() > 0
				? QString::fromStdString(output.humanName)
				: QString::fromStdString(output.name);
			nodeView->addSocketView(socketID, socketTitle, true);
			++socketID;
		}
	}

	// Make a default property - node name
	_propManager->newPropertyGroup(nodeID, "Common");
	_propManager->newProperty(nodeID, -1, EPropertyType::String,
		"Node name", nodeTitle, QString());

	PropertyID propID = 0;
	if(nodeConfig.pProperties)
	{
		_propManager->newPropertyGroup(nodeID, "Specific");

		while(nodeConfig.pProperties[propID].type != EPropertyType::Unknown)
		{
			auto& prop = nodeConfig.pProperties[propID];

			_propManager->newProperty(nodeID, propID, prop.type, 
				QString::fromStdString(prop.name), prop.initial, 
				QString::fromStdString(prop.uiHint));

			++propID;
		}
	}

	auto* propModel = _propManager->propertyModel(nodeID);
	Q_ASSERT(propModel);
	connect(propModel, &PropertyModel::propertyChanged,
		this, &Controller::changeProperty);

	connect(nodeView, &NodeView::mouseDoubleClicked,
		this, &Controller::mouseDoubleClickNodeView);

	// Cache node view and add it to a scene
	_nodeViews[nodeID] = nodeView;
	_nodeScene->addItem(nodeView);
}

void Controller::linkNodes(NodeSocketView* from, NodeSocketView* to)
{
	/// xXx: Add some checking to release
	Q_ASSERT(from);
	Q_ASSERT(from->nodeView());
	Q_ASSERT(to);
	Q_ASSERT(to->nodeView());	

	SocketAddress addrFrom(from->nodeView()->nodeKey(), from->socketKey(), true);
	SocketAddress addrTo(to->nodeView()->nodeKey(), to->socketKey(), false);

	/// xXx: give a reason
	if(!_nodeTree->linkNodes(addrFrom, addrTo))
	{
		showErrorMessage("[NodeTree] Couldn't linked given sockets");
		return;
	}

	// Create new link view 
	NodeLinkView* link = new NodeLinkView(from, to, nullptr);
	link->setDrawDebug(DEBUG_LINKS);
	from->addLink(link);
	to->addLink(link);

	// Cache it and add it to a scene
	_linkViews.append(link);
	_nodeScene->addItem(link);

	// Tag and execute
	_nodeTree->tagNode(addrTo.node);
	std::vector<NodeID> execList = _nodeTree->step();

	updatePreview(execList);
}

void Controller::unlinkNodes(NodeLinkView* linkView)
{
	/// xXx: Add some checking to release
	Q_ASSERT(linkView);
	Q_ASSERT(linkView->fromSocketView());
	Q_ASSERT(linkView->toSocketView());	

	const NodeSocketView* from = linkView->fromSocketView();
	const NodeSocketView* to = linkView->toSocketView();

	SocketAddress addrFrom(from->nodeView()->nodeKey(),
						   from->socketKey(), true);
	SocketAddress addrTo(to->nodeView()->nodeKey(), 
						 to->socketKey(), false);

	/// xXx: give a reason
	if(!_nodeTree->unlinkNodes(addrFrom, addrTo))
	{
		showErrorMessage("[NodeTree] Couldn't unlinked given sockets");
		return;
	}

	// Remove link view
	if(!_linkViews.removeOne(linkView))
	{
		showErrorMessage("[Controller] Couldn't removed link view");
		return;
	}
	_nodeScene->removeItem(linkView);
	delete linkView;

	// Tag and execute
	_nodeTree->tagNode(addrTo.node);
	std::vector<NodeID> execList = _nodeTree->step();

	updatePreview(execList);	
}


void Controller::deleteNode(NodeView* nodeView)
{
	Q_ASSERT(nodeView);

	// Try to remove the model first
	NodeID nodeID = NodeID(nodeView->nodeKey());

	if(!_nodeTree->removeNode(nodeID))
	{
		showErrorMessage("[NodeTree] Couldn't removed given node");
		return;
	}

	// Remove incoming and outgoing links
	// This should be done in cooperation with model but it would probably make the case harder
	/// xXx: Mark this as minor TODO
	///      We could retrieve the list of all removed links from NodeTree::removeNode 
	///      We would also need a method:
	///      NodeLinkView* resolveLinkView(const NodeLink& nodeLink);
	QMutableListIterator<NodeLinkView*> it(_linkViews);
	while(it.hasNext())
	{
		NodeLinkView* linkView = it.next();
		Q_ASSERT(linkView);

		if(linkView->inputConnecting(nodeView) ||
		   linkView->outputConnecting(nodeView))
		{
			auto socketView = linkView->toSocketView();
			Q_ASSERT(socketView);
			Q_ASSERT(socketView->nodeView());

			// tag affected node
			_nodeTree->tagNode(socketView->nodeView()->nodeKey());

			// remove link view
			_nodeScene->removeItem(linkView);
			it.remove();
			delete linkView;
		}
	}

	// Remove node view 
	_nodeScene->removeItem(nodeView);
	_nodeViews.remove(nodeID);
	if(_previewSelectedNodeView == nodeView)
		_previewSelectedNodeView = nullptr;
	nodeView->deleteLater();

	std::vector<NodeID> execList = _nodeTree->step();
	updatePreview(execList);	
}

void Controller::draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to)
{
	linkNodes(static_cast<NodeSocketView*>(from),
			  static_cast<NodeSocketView*>(to));
}

void Controller::draggingLinkStart(QGraphicsWidget* from)
{
	Q_UNUSED(from);
	_nodeScene->setDragging(true);
}

void Controller::draggingLinkStop(QGraphicsWidget* from)
{
	Q_UNUSED(from);
	_nodeScene->setDragging(false);
}

void Controller::contextMenu(const QPoint& globalPos,
	const QPointF& scenePos)
{
	QList<QGraphicsItem*> items = _nodeScene->items(scenePos, 
		Qt::ContainsItemShape, Qt::AscendingOrder);

	// If the user clicked onto empty space
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
					deleteNode(nodeView);
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

		// First remove all selected link
		for(int i = 0; i < selectedItems.size(); ++i)
		{
			auto item = selectedItems[i];

			if(item->type() == NodeLinkView::Type)
			{
				NodeLinkView* linkView = static_cast<NodeLinkView*>(item);
				unlinkNodes(linkView);
			}
			else
			{
				selectedNodeViews.append(item);
			}
		}

		// Then remove all selected nodes
		for(int i = 0; i < selectedNodeViews.size(); ++i)
		{
			auto item = selectedNodeViews[i];

			if(item->type() == NodeView::Type)
			{
				NodeView* nodeView = static_cast<NodeView*>(item);
				deleteNode(nodeView);
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

	std::vector<NodeID> execList = _nodeTree->step();
	updatePreview(execList);
}

void Controller::mouseDoubleClickNodeView(NodeView* nodeView)
{
	if(_previewSelectedNodeView != nodeView)
	{
		// Deselect previous one
		if(_previewSelectedNodeView != nullptr)
			_previewSelectedNodeView->selectPreview(false);
		_previewSelectedNodeView = nodeView;

		if(_previewSelectedNodeView != nullptr)
		{
			_previewSelectedNodeView->selectPreview(true);

			// Construct an artificial execList
			std::vector<NodeID> execList(1);
			execList[0] = _previewSelectedNodeView->nodeKey();
			updatePreview(execList);
		}
	}
}

void Controller::updatePreview(const std::vector<NodeID>& executedNodes)
{
	if(_previewSelectedNodeView != nullptr)
	{
		bool doUpdate = false;

		for(auto it = executedNodes.begin(); it != executedNodes.end(); ++it)
		{
			if(*it == _previewSelectedNodeView->nodeKey())
			{
				doUpdate = true;
				break;
			}
		}

		if(!doUpdate)
			return;

		/// xXx: For now we preview only first output socket
		///      This shouldn't be much a problem
		const cv::Mat& mat = _nodeTree->outputSocket(_previewSelectedNodeView->nodeKey(), 0);
		cv::Mat mat_;
			
		// Scale it up nicely
		/// xXx: In future we should use OpenGL and got free scaling

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

void Controller::showErrorMessage(const QString& message)
{
	QMessageBox::critical(nullptr, "mouve", message);
}

void Controller::sceneSelectionChanged()
{
	auto items = _nodeScene->selectedItems();

	if(items.count() == 1)
	{
		if(items[0]->type() == NodeView::Type)
		{
			auto nodeView = static_cast<NodeView*>(items[0]);
			auto propModel = _propManager->propertyModel(nodeView->nodeKey());

			if(propModel)
			{
				auto selectionModel = _ui->propertiesTreeView->selectionModel();
				_ui->propertiesTreeView->setModel(propModel);
				_ui->propertiesTreeView->expandToDepth(0);

				if(selectionModel)
					selectionModel->deleteLater();
			}
		}
	}
	// Deselected or selected more than 1
	else/* if(items.isEmpty()) */
	{
		auto selectionModel = _ui->propertiesTreeView->selectionModel();
		_ui->propertiesTreeView->setModel(nullptr);

		if(selectionModel)
			selectionModel->deleteLater();
	}
}

void Controller::changeProperty(NodeID nodeID,
								PropertyID propID, 
								const QVariant& newValue)
{
	//qDebug() << "Property changed," << nodeID << propID << newValue;

	// 'System' property
	if(propID < 0)
	{
		switch(propID)
		{
		/// TODO: Use enum
		case -1:
			{
				QString name = newValue.toString();
				std::string nameStd = name.toStdString();

				if(_nodeTree->nodeName(nodeID) == nameStd)
					// No change
					return;

				if(_nodeTree->resolveNode(nameStd) == InvalidNodeID)
				{
					_nodeTree->setNodeName(nodeID, nameStd);
					_nodeViews[nodeID]->setNodeViewName(name);
				}
				else
				{
					showErrorMessage("Name already taken");
				}
			}
			return;
		}
	}

	if(_nodeTree->nodeProperty(nodeID, propID, newValue))
	{
		_nodeTree->tagNode(nodeID);
		auto exs = _nodeTree->step();
		updatePreview(exs);
	}
	else
	{
		qDebug() << "[ERROR] bad value for nodeID:" << nodeID << 
			", propertyID:" << propID << ", newValue:" << newValue;
	}
}