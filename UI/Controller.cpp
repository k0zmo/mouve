#include "Controller.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "NodeLinkView.h"
#include "NodeSocketView.h"

#include "PropertyDelegate.h"
#include "PropertyManager.h"
#include "PropertyModel.h"

#include "LogView.h"

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
static const int maxImageWidth = 400;
static const int maxImageHeight = 400;

template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
	, _previewSelectedNodeView(nullptr)
	, _nodeScene(new NodeScene(this))
	, _nodeSystem(new NodeSystem())
	, _propManager(new PropertyManager(this))
	, _ui(new Ui::MainWindow())
	, _videoMode(false)
	, _currentlyPlaying(false)
	, _startWithInit(true)
	, _videoTimer(new QTimer(this))
{
	setupUi();

	// Set up a node scene
	/// xXx: Temporary
	///_nodeScene->setSceneRect(-200,-200,1000,600);

	connect(_nodeScene, &QGraphicsScene::selectionChanged,
		this, &Controller::sceneSelectionChanged);

	/// Qt bug concering scene->removeItem ?? Seems to fixed it
	_nodeScene->setItemIndexMethod(QGraphicsScene::NoIndex);

	_ui->graphicsView->setScene(_nodeScene);

	/// MB: Use eventFilter?
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

	connect(_videoTimer, &QTimer::timeout, this, &Controller::singleStep);
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

void Controller::setupUi()
{
	_ui->setupUi(this);

	/// TODO: use OpenGL
	_ui->outputPreview->setMinimumSize(maxImageWidth, maxImageHeight);
	_ui->outputPreview->setMaximumSize(maxImageWidth, maxImageHeight);

	// Menu bar actions
	_ui->actionQuit->setShortcuts(QKeySequence::Quit);
	connect(_ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);

	QAction* actionProperties = _ui->propertiesDockWidget->toggleViewAction();
	actionProperties->setShortcut(tr("Ctrl+1"));

	QAction* actionPreview = _ui->previewDockWidget->toggleViewAction();
	actionPreview->setShortcut(tr("Ctrl+2"));

	QAction* actionLog = _ui->logDockWidget->toggleViewAction();
	actionLog->setShortcut(tr("Ctrl+3"));

	// Create log view
	LogView* logView = new LogView(this);
	_ui->logDockWidget->setWidget(logView);
	// Hide log on default
	_ui->logDockWidget->hide();

	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	/// tabifyDockWidget(_ui->propertiesDockWidget, _ui->previewDockWidget);

	// Init menu bar and its 'view' menu
	_ui->menuView->addAction(actionProperties);
	_ui->menuView->addAction(actionPreview);
	_ui->menuView->addAction(actionLog);

	QAction* actionAboutQt = new QAction(
		QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), 
		tr("About &Qt"), this);
	actionAboutQt->setToolTip(tr("Show information about Qt"));
	actionAboutQt->setMenuRole(QAction::AboutQtRole);
	connect(actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	_ui->menuHelp->addAction(actionAboutQt);

	// Connect actions from a toolbar
	connect(_ui->actionSingleStep, &QAction::triggered, this, &Controller::singleStep);
	connect(_ui->actionAutoRefresh, &QAction::triggered, this, &Controller::autoRefresh);
	connect(_ui->actionPlay, &QAction::triggered, this, &Controller::play);
	connect(_ui->actionPause, &QAction::triggered, this, &Controller::pause);
	connect(_ui->actionStop, &QAction::triggered, this, &Controller::stop);

	_ui->actionPlay->setEnabled(false);
	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(false);

	// Init properties window
	PropertyDelegate* delegate = new PropertyDelegate(this);
	delegate->setImmediateUpdate(false);
	_ui->propertiesTreeView->setItemDelegateForColumn(1, delegate);
	_ui->propertiesTreeView->header()->setSectionResizeMode(
		QHeaderView::ResizeToContents);
}

bool Controller::isAutoRefresh()
{
	return _ui->actionAutoRefresh->isChecked();
}

void Controller::switchToVideoMode()
{
	if(_videoMode)
		return;

	_ui->actionAutoRefresh->setEnabled(false);
	_ui->actionPlay->setEnabled(true);
	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(false);

	_videoMode = true;
}

void Controller::switchToImageMode()
{
	if(!_videoMode)
		return;

	_ui->actionAutoRefresh->setEnabled(true);
	_ui->actionPlay->setEnabled(false);
	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(false);

	_videoMode = false;
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

	if(!_nodeTree->isTreeStateless())
		switchToVideoMode();
}

void Controller::addNodeView(const QString& nodeTitle,
	NodeID nodeID, const QPointF& scenePos)
{
	NodeConfig nodeConfig;
	if(!_nodeTree->nodeConfiguration(nodeID, nodeConfig))
	{
		showErrorMessage("[NodeTree] Error during querying node configuration");
		return;
	}

	NodeView* nodeView = new NodeView(nodeTitle);
	if(!nodeConfig.description.empty())
		nodeView->setToolTip(QString::fromStdString(nodeConfig.description));

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

	// Add node view to a scene
	_nodeScene->addItem(nodeView);
	nodeView->setData(NodeDataIndex::NodeKey, nodeID);
	nodeView->setPos(scenePos);
	_nodeViews[nodeID] = nodeView;
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

	processAutoRefresh();
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

	processAutoRefresh();
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

	if(_nodeTree->isTreeStateless())
		switchToImageMode();

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
			// remove link view
			_nodeScene->removeItem(linkView);
			it.remove();
			delete linkView;
		}
	}

	// Remove property model associated with removed node
	// We don't need to clear properties view first because
	// we clear selection - selectionChanged() - before
	_propManager->deletePropertyModel(nodeID);	

	// Remove node view 
	_nodeScene->removeItem(nodeView);
	_nodeViews.remove(nodeID);
	if(_previewSelectedNodeView == nodeView)
	{
		_previewSelectedNodeView = nullptr;
		updatePreview();
	}
	nodeView->deleteLater();

	processAutoRefresh();
}

void Controller::draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to)
{
	if(!_ui->graphicsView->isPseudoInteractive())
	{
		linkNodes(static_cast<NodeSocketView*>(from),
				  static_cast<NodeSocketView*>(to));
	}
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
		//QMenu* menuAddNode = menu.addMenu("Add node");
		foreach(QAction* a, _addNodesActions)
			menu.addAction(a);
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
			updatePreview();
		}
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
		// User selected one node 
		if(items[0]->type() == NodeView::Type)
		{
			auto nodeView = static_cast<NodeView*>(items[0]);
			auto propModel = _propManager->propertyModel(nodeView->nodeKey());

			if(propModel)
			{
				// Setting new model for a view enforces us to delete old selection model by ourselves
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
								const QVariant& newValue,
								bool* ok)
{
	qDebug() << "[INFO] Property changed! details: nodeID:" << nodeID << ", propID:" << propID << "newValue:" << newValue;

	// 'System' property
	if(propID < 0)
	{
		switch(propID)
		{
		/// TODO: Use enum
		case -1:
			if(newValue.type() == QMetaType::QString)
			{
				QString name = newValue.toString();
				std::string nameStd = name.toStdString();

				if(_nodeTree->nodeName(nodeID) == nameStd 
					|| name.isEmpty())
				{
					*ok = false;
					// No change
					return;
				}

				if(_nodeTree->resolveNode(nameStd) == InvalidNodeID)
				{
					_nodeTree->setNodeName(nodeID, nameStd);
					_nodeViews[nodeID]->setNodeViewName(name);
				}
				else
				{
					*ok = false;
					showErrorMessage("Name already taken");
				}
			}
			return;
		}
	}

	if(_nodeTree->nodeSetProperty(nodeID, propID, newValue))
	{
		_nodeTree->tagNode(nodeID);
		processAutoRefresh();
	}
	else
	{
		qWarning() << "[ERROR] bad value for nodeID:" << nodeID << 
			", propertyID:" << propID << ", newValue:" << newValue;
	}
}

void Controller::singleStep()
{
	/// TODO: Cleanup, this is only temporary (as well as "timer driven" video)
	if(!_videoMode)
	{
		_nodeTree->prepareList();
		_nodeTree->execute();

		if(shouldUpdatePreview(_nodeTree->executeList()))
			updatePreview();
	}
	else
	{
		setInteractive(false);

		_nodeTree->prepareList();
		_nodeTree->execute(_startWithInit);
		_startWithInit = false;

		_ui->actionStop->setEnabled(true);

		if(shouldUpdatePreview(_nodeTree->executeList()))
			updatePreview();
	}
}

void Controller::autoRefresh()
{
	/// TODO: change itemdelegate updateimmediate
	processAutoRefresh();
}

void Controller::play()
{
	if(!_currentlyPlaying)
	{
		_currentlyPlaying = true;
		_videoTimer->start(50);
	}

	_ui->actionPause->setEnabled(true);
	_ui->actionStop->setEnabled(true);
	_ui->actionPlay->setEnabled(false);
	_ui->actionSingleStep->setEnabled(false);

	setInteractive(false);
}

void Controller::pause()
{
	if(_currentlyPlaying)
	{
		_currentlyPlaying = false;
		_videoTimer->stop();
	}

	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(true);
	_ui->actionPlay->setEnabled(true);
	_ui->actionSingleStep->setEnabled(true);
}

void Controller::stop()
{
	if(_currentlyPlaying)
	{
		_currentlyPlaying = false;
		_videoTimer->stop();
	}

	_startWithInit = true;

	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(false);
	_ui->actionPlay->setEnabled(true);
	_ui->actionSingleStep->setEnabled(true);

	setInteractive(true);
}

void Controller::processAutoRefresh()
{
	// Execute if auto refresh is on
	if(!_videoMode && isAutoRefresh())
	{
		_nodeTree->execute();

		if(shouldUpdatePreview(_nodeTree->executeList()))
			updatePreview();
	}
}

bool Controller::shouldUpdatePreview(const std::vector<NodeID>& executedNodes)
{
	if(_previewSelectedNodeView)
	{
		for(auto nodeID : executedNodes)
		{
			if(nodeID == _previewSelectedNodeView->nodeKey())
			{
				return true;
			}
		}
	}

	return false;
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

		//if(mat.rows > maxImageHeight || 
		//   mat.cols > maxImageWidth)
		if(mat.rows > 0 && mat.cols > 0)
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
		/// TODO: Use somekind of dummy pixmap to indicate that preview is unavailable
		_ui->outputPreview->setPixmap(QPixmap());
	}
}

void Controller::setInteractive(bool allowed)
{
	_ui->graphicsView->setPseudoInteractive(allowed);

	if(allowed)
	{
		_nodeScene->setBackgroundBrush(QColor(64, 64, 64));
		_ui->propertiesTreeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
	}
	else
	{
		_nodeScene->setBackgroundBrush(QColor(34, 34, 34));
		_ui->propertiesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	}	
}