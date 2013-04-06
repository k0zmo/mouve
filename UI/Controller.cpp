#include "Controller.h"

// Model part
#include "Logic/NodeSystem.h"
#include "Logic/NodeTree.h"
#include "Logic/NodeType.h"
#include "Logic/NodeLink.h"
#include "Logic/Node.h"

// Views
#include "NodeView.h"
#include "NodeLinkView.h"
#include "NodeSocketView.h"
#include "NodeStyle.h"

// Properties
#include "PropertyDelegate.h"
#include "PropertyManager.h"
#include "PropertyModel.h"

// Additional widgets
#include "LogView.h"
#include "PreviewWidget.h"

#include <QMessageBox>
#include <QMenu>
#include <QLabel>
#include <QGraphicsWidget>
#include <QSettings>
#include <QDesktopWidget>
#include <QDebug>
#include <QProgressBar>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>

#include "ui_MainWindow.h"
#include "TreeWorker.h"

static const QString applicationTitle = QStringLiteral("mouve");
template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
	, _previewSelectedNodeView(nullptr)
	, _nodeScene(nullptr)
	, _propManager(new PropertyManager(this))
	, _nodeSystem(new NodeSystem())
	, _nodeTree(nullptr)
	, _workerThread(QThread())
	, _treeWorker(new TreeWorker())
	, _progressBar(nullptr)
	, _ui(new Ui::MainWindow())
	, _previewWidget(nullptr)
	, _contextMenuAddNodes(nullptr)
	, _stateLabel(nullptr)
	, _state(EState::Stopped)
	, _processing(false)
	, _videoMode(false)
	, _startWithInit(true)
	, _nodeTreeFilePath(QString())
	, _nodeTreeDirty(false)
{
	QCoreApplication::setApplicationName(applicationTitle);
	QCoreApplication::setOrganizationName(applicationTitle);

	setupUi();
	setupNodeTypesUi();
	populateAddNodeContextMenu();
	updateState(EState::Stopped);
	updateTitleBar();
	createNewNodeScene();

	// Context menu from node graphics view
	connect(_ui->graphicsView, &NodeEditorView::contextMenu,
		this, &Controller::contextMenu);
	// Key handler from node graphics view
	connect(_ui->graphicsView, &NodeEditorView::keyPress,
		this, &Controller::keyPress);

	_treeWorker->moveToThread(&_workerThread);

	connect(&_workerThread, &QThread::finished,
		_treeWorker, &QObject::deleteLater);
	connect(_treeWorker, &TreeWorker::completed,
		this, &Controller::updatePreview);
	connect(_treeWorker, &TreeWorker::error,
		this, &Controller::showErrorMessage);

	_workerThread.start();
}

Controller::~Controller()
{
	clearTreeView();

	_workerThread.quit();
	_workerThread.wait();

	delete _ui;
}

void Controller::closeEvent(QCloseEvent* event)
{
	// If processing, dtor of controller will wait and then shutdown itself
	if(_state == EState::Playing)
		stop();

	if (canQuit())
	{
		QSettings settings;
		settings.setValue("geometry", saveGeometry());
		settings.setValue("windowState", saveState());

		event->accept();
	} 
	else
	{
		event->ignore();
	}
}

void Controller::addNode(NodeTypeID nodeTypeID, const QPointF& scenePos)
{
	// Create new model
	std::string defaultNodeTitle = _nodeSystem->nodeTypeName(nodeTypeID);
	size_t pos = defaultNodeTitle.find_last_of('/');
	if(pos != std::string::npos && pos < defaultNodeTitle.size() - 1)
		defaultNodeTitle = defaultNodeTitle.substr(pos + 1);

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

	_nodeTreeDirty = true;
	updateTitleBar();

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

	QString nodeTypeName = QString::fromStdString(_nodeTree->nodeTypeName(nodeID));
	NodeView* nodeView = new NodeView(nodeTitle, nodeTypeName);
	if(!nodeConfig.description.empty())
		nodeView->setToolTip(QString::fromStdString(nodeConfig.description));

	// Add input sockets views to node view
	SocketID socketID = 0;
	if(nodeConfig.pInputSockets)
	{
		while(nodeConfig.pInputSockets[socketID].dataType != ENodeFlowDataType::Invalid)
		{
			auto& input = nodeConfig.pInputSockets[socketID];

			QString socketTitle = input.humanName.length() > 0
				? QString::fromStdString(input.humanName)
				: QString::fromStdString(input.name);
			nodeView->addSocketView(socketID, input.dataType, socketTitle, false);
			++socketID;
		}
	}

	// Add output sockets views to node view
	socketID = 0;
	if(nodeConfig.pOutputSockets)
	{
		while(nodeConfig.pOutputSockets[socketID].dataType != ENodeFlowDataType::Invalid)
		{
			auto& output = nodeConfig.pOutputSockets[socketID];

			QString socketTitle = output.humanName.length() > 0
				? QString::fromStdString(output.humanName)
				: QString::fromStdString(output.name);
			nodeView->addSocketView(socketID, output.dataType, socketTitle, true);
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
				QString::fromStdString(prop.name),
				_nodeTree->nodeProperty(nodeID, propID),
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

	// Set time info visibility on new nodes
	nodeView->setTimeInfoVisible(_ui->actionDisplayTimeInfo->isChecked());
}

void Controller::linkNodes(NodeID fromNodeID, SocketID fromSocketID,
		NodeID toNodeID, SocketID toSocketID)
{
	SocketAddress addrFrom(fromNodeID, fromSocketID, true);
	SocketAddress addrTo(toNodeID, toSocketID, false);

	/// TODO: give a reason
	if(!_nodeTree->linkNodes(addrFrom, addrTo))
	{
		showErrorMessage("[NodeTree] Couldn't linked given sockets");
		return;
	}

	/// TODO: Add some checking to release
	Q_ASSERT(_nodeViews[fromNodeID]);
	Q_ASSERT(_nodeViews[toNodeID]);

	linkNodesView(_nodeViews[fromNodeID]->outputSocketView(fromSocketID),
		_nodeViews[toNodeID]->inputSocketView(toSocketID));

	_nodeTreeDirty = true;
	updateTitleBar();
	processAutoRefresh();
}

void Controller::linkNodesView(NodeSocketView* from, NodeSocketView* to)
{
	/// TODO: Add some checking to release
	Q_ASSERT(from);
	Q_ASSERT(to);

	// Create new link view 
	NodeLinkView* link = new NodeLinkView(from, to, nullptr);
	link->setDrawDebug(false);
	from->addLink(link);
	to->addLink(link);

	// Cache it and add it to a scene
	_linkViews.append(link);
	_nodeScene->addItem(link);
}

void Controller::unlinkNodes(NodeLinkView* linkView)
{
	/// TODO: Add some checking to release
	Q_ASSERT(linkView);
	Q_ASSERT(linkView->fromSocketView());
	Q_ASSERT(linkView->toSocketView());	

	const NodeSocketView* from = linkView->fromSocketView();
	const NodeSocketView* to = linkView->toSocketView();

	SocketAddress addrFrom(from->nodeView()->nodeKey(),
						   from->socketKey(), true);
	SocketAddress addrTo(to->nodeView()->nodeKey(), 
						 to->socketKey(), false);

	/// TODO: give a reason
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

	_nodeTreeDirty = true;
	updateTitleBar();
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
	/// TODO: Mark this as minor TODO
	///       We could retrieve the list of all removed links from NodeTree::removeNode 
	///       We would also need a method:
	///       NodeLinkView* resolveLinkView(const NodeLink& nodeLink);
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
	//_nodeScene->removeItem(nodeView); // Calling this first with Bsp causes AV
	_nodeViews.remove(nodeID);
	if(_previewSelectedNodeView == nodeView)
	{
		_previewSelectedNodeView = nullptr;
		updatePreviewImpl();
	}
	//nodeView->deleteLater();
	delete nodeView;

	_nodeTreeDirty = true;
	updateTitleBar();
	processAutoRefresh();
}

void Controller::setupUi()
{
	_ui->setupUi(this);

	QSettings settings;
	QVariant varGeometry = settings.value("geometry");
	QVariant varState = settings.value("windowState");
	if(varGeometry.isValid() && varState.isValid())
	{
		restoreGeometry(varGeometry.toByteArray());
		restoreState(varState.toByteArray());
	}
	else
	{
		QDesktopWidget* desktop = QApplication::desktop();
		int screen = desktop->screenNumber(this);
		QRect rect(desktop->availableGeometry(screen));
		resize(int(rect.width() * .85), int(rect.height() * .85));
		move(rect.width()/2 - frameGeometry().width()/2,
			rect.height()/2 - frameGeometry().height()/2);

		showMaximized();
	}

	// Menu bar actions
	_ui->actionQuit->setShortcuts(QKeySequence::Quit);
	connect(_ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);
	
	QAction* actionNodes = _ui->nodesDockWidget->toggleViewAction();
	actionNodes->setShortcut(tr("Ctrl+1"));
	actionNodes->setIcon(QIcon(":/images/button-brick.png"));
	_ui->controlToolBar->addAction(actionNodes);

	QAction* actionProperties = _ui->propertiesDockWidget->toggleViewAction();
	actionProperties->setShortcut(tr("Ctrl+2"));
	actionProperties->setIcon(QIcon(":/images/button-gears.png"));

	QAction* actionPreview = _ui->previewDockWidget->toggleViewAction();
	actionPreview->setShortcut(tr("Ctrl+3"));
	actionPreview->setIcon(QIcon(":/images/button-image.png"));

	QAction* actionLog = _ui->logDockWidget->toggleViewAction();
	actionLog->setShortcut(tr("Ctrl+4"));

	// Create log view
	LogView* logView = new LogView(this);
	_ui->logDockWidget->setWidget(logView);
	connect(_treeWorker, &TreeWorker::error, logView, &LogView::critical);
	// Hide log on default
	_ui->logDockWidget->hide();

	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);

	tabifyDockWidget(_ui->propertiesDockWidget, _ui->nodesDockWidget);

	/// TODO: Bug in Qt 5.0.1 : Dock widget loses its frame and bar after undocked
	_ui->nodesDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
	_ui->propertiesDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
	_ui->previewDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
	_ui->logDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);

	// Init menu bar and its 'view' menu
	_ui->menuView->addAction(actionNodes);
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

	_ui->actionNewTree->setShortcut(QKeySequence(QKeySequence::New));
	_ui->actionOpenTree->setShortcut(QKeySequence(QKeySequence::Open));
	_ui->actionSaveTree->setShortcut(QKeySequence(QKeySequence::Save));

	// Connect actions from a toolbar
	connect(_ui->actionNewTree, &QAction::triggered, this, &Controller::newTree);
	connect(_ui->actionOpenTree, &QAction::triggered, this, &Controller::openTree);
	connect(_ui->actionSaveTree, &QAction::triggered, this, &Controller::saveTree);
	connect(_ui->actionSaveTreeAs, &QAction::triggered, this, &Controller::saveTreeAs);

	connect(_ui->actionSingleStep, &QAction::triggered, this, &Controller::singleStep);
	connect(_ui->actionAutoRefresh, &QAction::triggered, this, &Controller::autoRefresh);
	connect(_ui->actionPlay, &QAction::triggered, this, &Controller::play);
	connect(_ui->actionPause, &QAction::triggered, this, &Controller::pause);
	connect(_ui->actionStop, &QAction::triggered, this, &Controller::stop);

	connect(_ui->actionDisplayTimeInfo, &QAction::triggered, this, &Controller::displayNodeTimeInfo);
	connect(_ui->actionFitToView, &QAction::triggered, this, &Controller::fitToView);

	_ui->actionPlay->setEnabled(false);
	_ui->actionPause->setEnabled(false);
	_ui->actionStop->setEnabled(false);

	// Init properties window
	PropertyDelegate* delegate = new PropertyDelegate(this);
	delegate->setImmediateUpdate(true);
	_ui->propertiesTreeView->setItemDelegateForColumn(1, delegate);
	_ui->propertiesTreeView->header()->setSectionResizeMode(
		QHeaderView::ResizeToContents);

	_ui->controlToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	// Init preview window
	_previewWidget = new PreviewWidget(this);
	_previewWidget->showDummy();
	_ui->previewDockWidget->setWidget(_previewWidget);

	// Init status bar
	_stateLabel = new QLabel(this);
	_ui->statusBar->addPermanentWidget(_stateLabel);

	// Init worker thread progress bar
	_progressBar = new QProgressBar(this);
	_progressBar->setFixedSize(200, _ui->statusBar->sizeHint().height() - 6);
	_progressBar->setRange(0, 0);
	_ui->statusBar->setFixedHeight(_ui->statusBar->sizeHint().height());
	_ui->statusBar->addWidget(_progressBar);
	_ui->statusBar->removeWidget(_progressBar);
}

void Controller::setupNodeTypesUi()
{
	connect(_ui->nodesTreeWidget, &QTreeWidget::itemDoubleClicked, 
		[=](QTreeWidgetItem* item, int column)
		{
			QGraphicsView* view = _ui->graphicsView;
			QPointF centerPos = view->mapToScene(view->viewport()->rect().center());
			NodeTypeID typeId = item->data(column, Qt::UserRole).toUInt();
			if(typeId != InvalidNodeTypeID)
				addNode(item->data(column, Qt::UserRole).toUInt(), centerPos);
		});

	QList<QTreeWidgetItem*> treeItems;
	auto nodeTypeIterator = _nodeSystem->createNodeTypeIterator();
	NodeTypeIterator::NodeTypeInfo info;
	while(nodeTypeIterator->next(info))
	{
		QString typeName = QString::fromStdString(info.typeName);
		QStringList tokens = typeName.split('/');
		addNodeTypeTreeItem(info.typeID, tokens, treeItems);
	}

	// Set proper item flags
	for(QTreeWidgetItem* item : treeItems)
	{
		Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
		// Is it leaf?
		if(item->childCount() == 0)
			flags |=  Qt::ItemIsDragEnabled;			
		item->setFlags(flags);
	}

	_ui->nodesTreeWidget->insertTopLevelItems(0, treeItems);
}

void Controller::addNodeTypeTreeItem(NodeTypeID typeId, const QStringList& tokens,
									 QList<QTreeWidgetItem*>& treeItems)
{
	QTreeWidgetItem* parent = nullptr;

	auto findItem = [=](const QString& text) -> QTreeWidgetItem*
	{
		for(const auto item : treeItems)
			if(item->text(0) == text)
				return item;
		return nullptr;
	};

	auto findChild = [=](const QTreeWidgetItem* parent,
		const QString& text) -> QTreeWidgetItem*
	{
		for(int i = 0; i < parent->childCount(); ++i)
		{
			QTreeWidgetItem* item = parent->child(i);
			if(item->text(0) == text)
				return item;
		}
		return nullptr;
	};

	if(tokens.count() > 1)
	{
		for(int level = 0; level < tokens.count() - 1; ++level)
		{
			QString parentToken = tokens[level];
			QTreeWidgetItem* p = level == 0 
				? findItem(parentToken)
				: findChild(parent, parentToken);
			if(!p)
			{
				p = new QTreeWidgetItem(parent, QStringList(parentToken));
				if(level == 0)
					treeItems.append(p);
			}
			parent = p;
		}
	}

	QTreeWidgetItem* item = new QTreeWidgetItem(parent);
	item->setText(0, tokens.last());
	item->setData(0, Qt::UserRole, typeId);

	treeItems.append(item);
}

void Controller::populateAddNodeContextMenu()
{
	if(_contextMenuAddNodes)
		_contextMenuAddNodes->deleteLater();

	// Init context menu for adding nodes
	_contextMenuAddNodes = new QMenu(this);
	QList<QMenu*> subMenuList;
	subMenuList.append(_contextMenuAddNodes);

	QTreeWidgetItemIterator iter(_ui->nodesTreeWidget);
	while(*iter)
	{
		QString text = (*iter)->text(0);
		text = text.replace("&", "&&");
		QTreeWidgetItem* i = *iter;
		int level = 1;
		while(i = i->parent())
			++level;

		// Bottom level items
		if((*iter)->childCount() == 0)
		{
			NodeTypeID typeId = (*iter)->data(0, Qt::UserRole).toUInt();
			QAction* action = new QAction(text, this);
			action->setData(typeId);

			// Level starts cunting from 1 so at worst - it's gonnaa point to 0-indexed which is main menu
			subMenuList[level-1]->addAction(action);
		}
		// Categories
		else
		{
			// Add new category to context menu
			QMenu* subMenu = subMenuList[level-1]->addMenu(text);

			if(level >= subMenuList.count())
			{
				subMenuList.append(subMenu);
			}
			else if(subMenuList[level]->title() != text)
			{
				subMenuList[level] = subMenu;
				while(subMenuList.count() > level + 1)
					subMenuList.removeLast();
			}
		}

		iter++;
	}
}

void Controller::showErrorMessage(const QString& message)
{
	QMessageBox::critical(nullptr, applicationTitle, message);
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

void Controller::updateState(EState state)
{
	_state  = state;

	switch(_state)
	{
	case EState::Stopped:
		if(_processing)
			_stateLabel->setText("Stopped (wait)");
		else
			_stateLabel->setText("Stopped (editing allowed)");
		break;
	case EState::Playing:
		_stateLabel->setText("Playing");
		break;
	case EState::Paused:
		if(_processing)
			_stateLabel->setText("Paused (wait)");
		else
			_stateLabel->setText("Paused");
		break;
	}
}

bool Controller::isAutoRefresh()
{
	return _ui->actionAutoRefresh->isChecked();
}

void Controller::processAutoRefresh()
{
	// Execute if auto refresh is on
	if(!_videoMode && isAutoRefresh() && treeIdle())
		singleStep();
}

void Controller::queueProcessing(bool withInit)
{
	_processing = true;
	QMetaObject::invokeMethod(_treeWorker, 
		"process", Qt::QueuedConnection,
		Q_ARG(bool, withInit));
}

bool Controller::shouldUpdatePreview(const std::vector<NodeID>& executedNodes)
{
	if(_previewSelectedNodeView)
	{
		if(executedNodes.empty())
		{
			// Invalidate if selected node lost its input(s) 
			if(_nodeTree->taggedButNotExecuted(_previewSelectedNodeView->nodeKey()))
				return true;
		}

		for(auto nodeID : executedNodes)
		{
			if(nodeID == _previewSelectedNodeView->nodeKey())
				return true;
		}
	}

	return false;
}

void Controller::updatePreviewImpl()
{
	if(_previewSelectedNodeView != nullptr
		&& _previewSelectedNodeView->outputSocketCount() > 0)
	{
		NodeID nodeID = _previewSelectedNodeView->nodeKey();
		SocketID socketID = _previewSelectedNodeView->previewSocketID();

		const NodeFlowData& outputData = _nodeTree->outputSocket(nodeID, socketID);

		if(outputData.isValid())
		{	
			switch(outputData.type())
			{
			case ENodeFlowDataType::Image:
				_previewWidget->show(outputData.getImage());
				return;

			case ENodeFlowDataType::ImageRgb:
				_previewWidget->show(outputData.getImageRgb());
				return;
			default:
				break;
			}
		}
	}

	// Fallback
	_previewWidget->showDummy();
}

void Controller::setInteractive(bool allowed)
{
	if(allowed)
	{
		if(!_videoMode)
		{
			_ui->actionSingleStep->setEnabled(true);
			_ui->actionAutoRefresh->setEnabled(true);
			_ui->statusBar->removeWidget(_progressBar);
		}
		_nodeScene->setBackgroundBrush(NodeStyle::SceneBackground);
		_ui->propertiesTreeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
	}
	else
	{
		if(_videoMode)
		{
			_nodeScene->setBackgroundBrush(NodeStyle::SceneBlockedBackground);
		}
		else
		{
			_ui->actionSingleStep->setEnabled(false);
			_ui->actionAutoRefresh->setEnabled(false);
			_ui->statusBar->addWidget(_progressBar);
			_progressBar->show();
		}
		_ui->propertiesTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	}

	_ui->actionNewTree->setEnabled(allowed);
	_ui->actionOpenTree->setEnabled(allowed);
	_ui->actionSaveTree->setEnabled(allowed);
	_ui->actionSaveTreeAs->setEnabled(allowed);
}

void Controller::updateTitleBar()
{
	QString tmp = _nodeTreeFilePath.isEmpty()
		? tr("Untitled")
		: QFileInfo(_nodeTreeFilePath).fileName();
	QString windowTitle = applicationTitle + " - ";
	if(_nodeTreeDirty)
		windowTitle += "*";
	setWindowTitle(windowTitle + tmp);
}

bool Controller::canQuit()
{
	if(_nodeTreeDirty)
	{
		QString message = QString("Save \"%1\" ?")
			.arg((_nodeTreeFilePath.isEmpty()
				? tr("Untitled")
				: _nodeTreeFilePath));

		QMessageBox::StandardButton reply= QMessageBox::question(
			this, tr("Save unsaved changes"), message, 
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (reply == QMessageBox::Yes)
		{
			if(!saveTree())
				return false;
		}
		else if(reply == QMessageBox::Cancel)
		{
			return false;
		}
	}

	return true;
}

void Controller::clearTreeView()
{
	// NOTE: This order is important 
	// If node views would be first deleted then during a link view destruction 
	// we would get AV because it tries to access already nonexistent 
	// socket view (which is removed with its parent - nodeview)
	// This isn't pretty but for now it's a must

	for(NodeLinkView* link : _linkViews)
	{
		// Removing item from scene before removing the item 
		// itself is not required but it should be faster altogether
		_nodeScene->removeItem(link);
		delete link;
	}
	_linkViews.clear();	

	for(NodeView* node : _nodeViews.values())
	{
		_nodeScene->removeItem(node);
		delete node;
	}
	_nodeViews.clear();
}

void Controller::createNewNodeScene()
{
	// Create new tree model
	_nodeTree = _nodeSystem->createNodeTree();
	_treeWorker->setNodeTree(_nodeTree);

	// Create new tree view (node scene)
	_nodeScene = new QGraphicsScene(this);
	_nodeScene->setBackgroundBrush(NodeStyle::SceneBackground);
	connect(_nodeScene, &QGraphicsScene::selectionChanged,
		this, &Controller::sceneSelectionChanged);
	_ui->graphicsView->setScene(_nodeScene);
}

void Controller::createNewTree()
{
	// Deselect current proper model 
	_nodeScene->clearSelection();

	// Remove leftovers
	_previewSelectedNodeView = nullptr;
	_nodeTreeDirty = false;
	_nodeTreeFilePath = QString();

	_propManager->clear();

	// We can't rely on scene destruction because its order doesn't satisfy us
	clearTreeView();
	_nodeScene->deleteLater();

	// Create new tree (view and model)
	createNewNodeScene();

	updateTitleBar();
	updatePreviewImpl();
	switchToImageMode();
}

bool Controller::saveTreeToFile(const QString& filePath)
{
	if(saveTreeToFileImpl(filePath))
	{
		_nodeTreeFilePath = filePath;
		_nodeTreeDirty = false;
		updateTitleBar();
		qDebug() << "Tree successfully saved to file:" << filePath;

		return true;
	}
	else
	{
		showErrorMessage("Error occured during saving file! Check logs for more details.");
		qCritical() << "Couldn't save node tree to file:" << filePath;

		return false;
	}
}

bool Controller::saveTreeToFileImpl(const QString& filePath)
{
	// Add view part to deserialized part
	QJsonArray jsonScene;
	for(auto nvit = _nodeViews.begin(); nvit != _nodeViews.end(); ++nvit)
	{
		QJsonObject jsonSceneElem;

		QPointF nodePos = _nodeViews[nvit.key()]->scenePos();
		jsonSceneElem.insert(QStringLiteral("nodeId"), nvit.key());
		jsonSceneElem.insert(QStringLiteral("scenePosX"), nodePos.x());
		jsonSceneElem.insert(QStringLiteral("scenePosY"), nodePos.y());

		jsonScene.append(jsonSceneElem);
	}

	QJsonObject jsonTree = _nodeTree->serializeJson();
	jsonTree.insert(QStringLiteral("scene"), jsonScene);

	QJsonDocument doc(jsonTree);
	QByteArray textJson = doc.toJson();

	QFile file(filePath);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	file.write(textJson);
	return true;
}

bool Controller::openTreeFromFileImpl(const QString& filePath)
{
	QFile file(filePath);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QByteArray fileContents = file.readAll();
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(fileContents, &error);
	if(error.error != QJsonParseError::NoError)
	{
		showErrorMessage("Error occured during parsing file! Check logs for more details.");
		qCritical() << "Couldn't open node tree from file:" << filePath
			<< " details:" << error.errorString() << "in" << error.offset << "character";
		return false;
	}

	if(!doc.isObject())
	{
		showErrorMessage("Error occured during parsing file! Check logs for more details.");
		qCritical() << "Couldn't open node tree from file:" << filePath
			<< " details: root value isn't JSON object";
		return false;
	}

	QJsonObject jsonTree = doc.object();

	// Method used in loading nodes makes them lose their original NodeID
	std::map<NodeID, NodeID> mapping;
	if(!_nodeTree->deserializeJson(jsonTree, &mapping))
		return false;

	// Deserialize view part
	QJsonArray jsonScene = jsonTree["scene"].toArray();
	QVariantList scene = jsonScene.toVariantList();

	for(auto& sceneElem : scene)
	{
		QVariantMap elemMap = sceneElem.toMap();

		NodeID nodeID = elemMap["nodeId"].toUInt();
		NodeID mappedNodeID = mapping[nodeID];
		double scenePosX = elemMap["scenePosX"].toDouble();
		double scenePosY = elemMap["scenePosY"].toDouble();

		// Create new view associated with the model
		if(!_nodeViews.contains(mappedNodeID))
		{
			if(!_nodeTree->nodeTypeID(mappedNodeID) == InvalidNodeTypeID)
			{
				QString nodeName = QString::fromStdString(_nodeTree->nodeName(mappedNodeID));
				addNodeView(nodeName, mappedNodeID, QPointF(scenePosX, scenePosY));
			}
			else
			{
				qWarning() << "Couldn't find node for scene element with node id:" << nodeID;
			}
		}
		else
		{
			qWarning() << "More than one scene element with the same node id:" << nodeID;
		}
	}

	if(size_t(_nodeViews.count()) != _nodeTree->nodesCount())
	{
		qWarning("Some scene elements were missing, adding them to scene origin."
			"You can fix it by resaving the file");
		
		auto nodeIt = _nodeTree->createNodeIterator();
		NodeID nodeID;
		while(nodeIt->next(nodeID))
		{
			NodeID mappedNodeID = mapping[nodeID];
			if(!_nodeViews.contains(mappedNodeID))
			{
				QString nodeName = QString::fromStdString(_nodeTree->nodeName(mappedNodeID));
				addNodeView(nodeName, mappedNodeID, QPointF(0, 0));
			}
		}
	}

	// Add link views to the scene 
	auto linkIt = _nodeTree->createNodeLinkIterator();
	NodeLink nodeLink;
	while(linkIt->next(nodeLink))
	{
		NodeView* fromNodeView = _nodeViews[nodeLink.fromNode];
		NodeView* toNodeView = _nodeViews[nodeLink.toNode];

		if(fromNodeView && toNodeView)
		{
			NodeSocketView* fromSocketView = fromNodeView->outputSocketView(nodeLink.fromSocket);
			NodeSocketView* toSocketView = toNodeView->inputSocketView(nodeLink.toSocket);

			if(fromSocketView && toSocketView)
				linkNodesView(fromSocketView, toSocketView);
		}
	}

	if(_nodeTree->isTreeStateless())
		switchToImageMode();
	else
		switchToVideoMode();

	return true;
}

void Controller::contextMenu(const QPoint& globalPos,
	const QPointF& scenePos)
{
	_ui->statusBar->showMessage(QString("Scene position: %1, %2")
		.arg(scenePos.x())
		.arg(scenePos.y())
	);

	QList<QGraphicsItem*> items = _nodeScene->items(scenePos, 
		Qt::ContainsItemShape, Qt::AscendingOrder);

	// If the user clicked onto empty space
	if(items.isEmpty())
	{
		if(treeIdle())
		{
			QAction* ret = _contextMenuAddNodes->exec(globalPos);
			if(ret != nullptr)
				addNode(ret->data().toInt(), scenePos);
		}
	}
	else
	{
		for(int i = 0; i < items.size(); ++i)
		{
			auto item = items[i];

			if(item->type() == NodeView::Type)
			{
				QMenu menu;
				QAction actionRemoveNode("Remove node", nullptr);
				if(treeIdle())
					menu.addAction(&actionRemoveNode);

				NodeView* nodeView = static_cast<NodeView*>(item);
				int previewsCount = nodeView->outputSocketCount();
				if(previewsCount > 0)
				{
					if(_state == EState::Stopped && !_processing)
						menu.addSeparator();

					int curr = nodeView->previewSocketID();
					for(int i = 0; i < previewsCount; ++i)
					{
						QAction* actionPreviewSocket = new QAction
							(QString("Preview socket: %1").arg(i), &menu);
						actionPreviewSocket->setCheckable(true);
						actionPreviewSocket->setChecked(i == curr);
						actionPreviewSocket->setData(i);
						connect(actionPreviewSocket, &QAction::triggered,
							[=](bool checked)
							{
								if(checked)
								{
									nodeView->setPreviewSocketID
										(actionPreviewSocket->data().toInt());
									updatePreviewImpl();
								}
							});
						menu.addAction(actionPreviewSocket);
					}
				}

				QAction* ret = menu.exec(globalPos);
				if(ret == &actionRemoveNode && treeIdle())
					deleteNode(nodeView);
				return;
			}
		}
	}
}

void Controller::keyPress(QKeyEvent* event)
{
	if(!treeIdle())
		return;

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
#if defined(QT_DEBUG)
	// Debugging feature
	else if(event->key() == Qt::Key_T
		&& event->modifiers() & Qt::ControlModifier)
	{
		// tag all nodes if ctrl+T was pressed
		auto iter = _nodeTree->createNodeIterator();
		NodeID nodeID;
		while(iter->next(nodeID))
			_nodeTree->tagNode(nodeID);
	}
	else if(event->key() == Qt::Key_G
		&& event->modifiers() & Qt::ControlModifier)
	{
		if(!canQuit())
			return;

		QString filePath = "test.tree";

		_ui->actionAutoRefresh->setChecked(false);
		createNewTree();

		if(openTreeFromFileImpl(filePath))
		{
			_nodeTreeFilePath = filePath;
			_nodeTreeDirty = false;
			updateTitleBar();
			qDebug() << "Tree successfully opened from file:" << filePath;
		}
		else
		{
			showErrorMessage("Error occured during opening file! Check logs for more details.");
			qCritical() << "Couldn't open node tree from file:" << filePath;
		}
	}
#endif
}

void Controller::draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to)
{
	/// TODO: Do not let connector view to even start dragging
	if(!treeIdle())
		return;

	auto fromSocket = static_cast<NodeSocketView*>(from);
	auto toSocket = static_cast<NodeSocketView*>(to);

	Q_ASSERT(fromSocket);
	Q_ASSERT(toSocket);
	Q_ASSERT(fromSocket->nodeView());
	Q_ASSERT(toSocket->nodeView());

	linkNodes(fromSocket->nodeView()->nodeKey(),
		fromSocket->socketKey(),
		toSocket->nodeView()->nodeKey(),
		toSocket->socketKey());
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
			updatePreviewImpl();
		}
	}
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

				return;
			}
		}
	}

	// Deselected or selected more than 1 items or selected something else than node 
	auto selectionModel = _ui->propertiesTreeView->selectionModel();
	_ui->propertiesTreeView->setModel(nullptr);

	if(selectionModel)
		selectionModel->deleteLater();
}

void Controller::changeProperty(NodeID nodeID,
								PropertyID propID, 
								const QVariant& newValue,
								bool* ok)
{
	if(!treeIdle())
	{
		*ok = false;
		return;
	}

	//qDebug() << "Property changed! details: nodeID:" << 
	//	nodeID << ", propID:" << propID << "newValue:" << newValue;

	// 'System' property
	if(propID < 0)
	{
		switch(propID)
		{
		/// TODO: Use enum
		case -1:
			if(newValue.type() == QVariant::String)
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
					_nodeTreeDirty = true;
					updateTitleBar();
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
		_nodeTreeDirty = true;
		updateTitleBar();
		processAutoRefresh();
	}
	else
	{
		*ok = false;
		qWarning() << "Bad value for nodeID:" << nodeID << 
			", propertyID:" << propID << ", newValue:" << newValue;
	}
}

void Controller::newTree()
{
	if(canQuit())
		createNewTree();
}

void Controller::openTree()
{
	if(!canQuit())
		return;

	QString filePath = QFileDialog::getOpenFileName(
		this, tr("Open file"), QString(), "Node tree files (*.tree)");
	if(filePath.isEmpty())
		return;

	_ui->actionAutoRefresh->setChecked(false);
	createNewTree();

	if(openTreeFromFileImpl(filePath))
	{
		_nodeTreeFilePath = filePath;
		_nodeTreeDirty = false;
		updateTitleBar();
		qDebug() << "Tree successfully opened from file:" << filePath;
	}
	else
	{
		showErrorMessage("Error occured during opening file! Check logs for more details.");
		qCritical() << "Couldn't open node tree from file:" << filePath;
	}
}

bool Controller::saveTree()
{
	// if current tree isn't associated with any file we need to act as "save as"
	if(_nodeTreeFilePath.isEmpty())
	{
		return saveTreeAs();
	}

	return saveTreeToFile(_nodeTreeFilePath);
}

bool Controller::saveTreeAs()
{
	QString filePath = QFileDialog::getSaveFileName(
		this, tr("Save file as..."),QString(), "Node tree files (*.tree)"); 
	if(filePath.isEmpty())
		return false;

	return saveTreeToFile(filePath);
}

void Controller::updatePreview(bool res)
{
	// Workerer reported something gone wrong
	if(!res)
	{
		_processing = false;
		if(!_videoMode)
			setInteractive(true);
		else
			stop();
		return;
	}

	std::vector<NodeID> execList = _nodeTree->executeList();

	// Update preview window if necessary
	if(shouldUpdatePreview(execList))
		updatePreviewImpl();

	// Job is done - enable editing
	_processing = false;

	// Update time info on nodes
	auto iter = _nodeTree->createNodeIterator();
	NodeID nodeID;
	while(auto node = iter->next(nodeID))
	{
		QString text = QString::number(node->timeElapsed());
		text += QStringLiteral(" ms");
		_nodeViews[nodeID]->setTimeInfo(text);
	}

	if(!_videoMode)
	{
		setInteractive(true);
	}
	else
	{
		// Peek if it's a last "frame"
		if(_state != EState::Stopped)
		{
			auto executeList = _nodeTree->prepareList();
			if(executeList.empty())
			{
				stop();
				return;
			}
		}
		
		switch(_state)
		{
		case EState::Paused:
			updateControlButtonState(EState::Paused);
			updateState(EState::Paused);
			break;
		case EState::Stopped:
			updateControlButtonState(EState::Stopped);
			updateState(EState::Stopped);
			setInteractive(true);

			/// Could  be somehow done on worker thread
			/// (some nodes could finish a bit "long")
			_nodeTree->notifyFinish();
			// On next time start with init step
			_startWithInit = true;
			break;
		case EState::Playing:
			// Keep playing 
			queueProcessing(false);
			break;
		}
	}
}

void Controller::singleStep()
{
	if(_processing)
		return;

	auto executeList = _nodeTree->prepareList();
	if(executeList.empty())
	{
		// Should we invalidate current preview?
		if(shouldUpdatePreview(executeList))
			updatePreviewImpl();
		return;
	}

	// Single step in image mode
	if(!_videoMode)
	{
		setInteractive(false);
		queueProcessing(false);
	}
	// Single step in video mode 
	else
	{
		// Start processing (on worker thread)
		queueProcessing(_startWithInit);
		_startWithInit = false;

		if(_state == EState::Stopped)
		{
			// If we started playing using "Step" button we need to update these
			updateState(EState::Paused);
			setInteractive(false);
			_ui->actionStop->setEnabled(true);
		}
	}
}

void Controller::autoRefresh()
{
	processAutoRefresh();
}

void Controller::play()
{
	if(_state != EState::Playing)
	{
		updateState(EState::Playing);
		updateControlButtonState(EState::Playing);
		setInteractive(false);

		// Start processing (on worker thread)
		queueProcessing(_startWithInit);
		_startWithInit = false;
	}
}

void Controller::pause()
{
	if(_state == EState::Playing)
	{
		// Next time process() won't be invoke
		updateState(EState::Paused);
	}
}

void Controller::stop()
{
	if(_state != EState::Stopped)
	{
		if(_processing)
		{
			updateState(EState::Stopped);
		}
		else
		{
			// Worker thread is idle, we need to update stuff from here
			updateState(EState::Stopped);
			updateControlButtonState(EState::Stopped);
			setInteractive(true);

			// On next time start with init step
			_startWithInit = true;

			/// Could  be somehow done on worker thread
			/// (some nodes could finish a bit "long")
			_nodeTree->notifyFinish();
		}
	}
}

void Controller::fitToView()
{
	QList<QGraphicsItem*> items = _nodeScene->items();
	if(items.isEmpty())
		return;

	QRectF sceneRect = items[0]->sceneBoundingRect();

	for(auto* item : items)
	{
		const QRectF& brect = item->sceneBoundingRect();
		sceneRect.setTop(qMin(sceneRect.top(), brect.top()));
		sceneRect.setLeft(qMin(sceneRect.left(), brect.left()));
		sceneRect.setBottom(qMax(sceneRect.bottom(), brect.bottom()));
		sceneRect.setRight(qMax(sceneRect.right(), brect.right()));
	}

	_ui->graphicsView->fitInView(sceneRect, Qt::KeepAspectRatio);

	// We assume there's no rotation
	QTransform t = _ui->graphicsView->transform();
	
	_ui->graphicsView->setZoom(t.m11());
}

void Controller::updateControlButtonState(EState state)
{
	// Should only be used when in video mode
	if(!_videoMode)
		return;

	switch(state)
	{
	case EState::Stopped:
		_ui->actionPause->setEnabled(false);
		_ui->actionStop->setEnabled(false);
		_ui->actionPlay->setEnabled(true);
		_ui->actionSingleStep->setEnabled(true);
		break;
	case EState::Paused:
		if(_videoMode)
		_ui->actionPause->setEnabled(false);
		_ui->actionStop->setEnabled(true);
		_ui->actionPlay->setEnabled(true);
		_ui->actionSingleStep->setEnabled(true);
		break;
	case EState::Playing:
		_ui->actionPause->setEnabled(true);
		_ui->actionStop->setEnabled(true);
		_ui->actionPlay->setEnabled(false);
		_ui->actionSingleStep->setEnabled(false);
		break;
	}
}

bool Controller::treeIdle()
{
	return _state == EState::Stopped && !_processing;
}

void Controller::displayNodeTimeInfo(bool checked)
{
	auto iter = _nodeViews.begin();
	for( ; iter != _nodeViews.end(); ++iter)
		iter.value()->setTimeInfoVisible(checked);
}