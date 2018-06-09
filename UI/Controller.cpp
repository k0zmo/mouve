/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "Controller.h"

#include "Kommon/Utils.h"
#include "Kommon/ModulePath.h"
#include "Kommon/NestedException.h"

// Model part
#include "Logic/NodeSystem.h"
#include "Logic/NodeTree.h"
#include "Logic/NodeType.h"
#include "Logic/NodeLink.h"
#include "Logic/Node.h"
#include "Logic/NodeTreeSerializer.h"

// Modules
#include "Logic/OpenCL/IGpuNodeModule.h"
#include "Logic/OpenCL/GpuException.h"
#include "Logic/Jai/IJaiNodeModule.h"
#include "GpuChoiceDialog.h"

// Node editor
#include "NodeEditor/NodeView.h"
#include "NodeEditor/NodeLinkView.h"
#include "NodeEditor/NodeSocketView.h"
#include "NodeEditor/NodeStyle.h"
#include "NodeEditor/NodeToolTip.h"

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
#include <QProgressDialog>
#include <QGraphicsSceneHoverEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>

#include "ui_MainWindow.h"
#include "TreeWorker.h"

#include <future>
#include <thread>

template<> Controller* Singleton<Controller>::_singleton = nullptr;

namespace {
void logException(const std::exception& ex)
{
    try { std::rethrow_if_nested(ex); }
    catch (const std::exception& ex) { logException(ex); }
    catch (...) {}
    qCritical() << "Error:" << ex.what();
}
}

Controller::Controller(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , _previewSelectedNodeView(nullptr)
    , _nodeToolTip(nullptr)
    , _nodeScene(nullptr)
    , _propManager(new PropertyManager(this))
    , _nodeSystem(new NodeSystem())
    , _nodeTree(nullptr)
    // odd: below crashes in gcc 
    //, _workerThread(QThread())
    , _treeWorker(new TreeWorker())
    , _progressBar(nullptr)
    , _gpuModule(createGpuModule())
    , _jaiModule(createJaiModule())
    , _ui(new Ui::MainWindow())
    , _previewWidget(nullptr)
    , _contextMenuAddNodes(nullptr)
    , _totalTimeLabel(nullptr)
    , _stateLabel(nullptr)
    , _state(EState::Stopped)
    , _processing(false)
    , _videoMode(false)
    , _startWithInit(true)
    , _nodeTreeFilePath(QString())
    , _nodeTreeDirty(false)
    , _showTooltips(false)
{
    setupUi();

    // Lookup for plugins in ./plugins directory
    pluginLookUp();
    qDebug() << "Number of available nodes: " << _nodeSystem->numRegisteredNodeTypes();

    setupNodeTypesUi();
    populateAddNodeContextMenu();
    updateState(EState::Stopped);
    updateTitleBar();
    createNewNodeScene();

    auto menuModules = _ui->menuBar->addMenu("Modules");

    initGpuModule(menuModules);	
    initJaiModule(menuModules);

    if(menuModules->actions().isEmpty())
        menuModules->setEnabled(false);

    // Context menu from node graphics view
    connect(_ui->graphicsView, &NodeEditorView::contextMenu,
        this, &Controller::nodeEditorContextMenu);
    // Key handler from node graphics view
    connect(_ui->graphicsView, &NodeEditorView::keyPress,
        this, &Controller::nodeEditorKeyPressed);

    _treeWorker->moveToThread(&_workerThread);

    connect(&_workerThread, &QThread::finished,
        _treeWorker, &QObject::deleteLater);
    connect(_treeWorker, &TreeWorker::completed,
        this, &Controller::updatePreview);
    connect(_treeWorker, &TreeWorker::error,
        this, &Controller::showErrorMessage);
    connect(_treeWorker, &TreeWorker::badConnection,
        this, &Controller::showBadConnection);

    _workerThread.start();
}

Controller::~Controller()
{
    clearTreeView();

    _workerThread.quit();
    _workerThread.wait();

    _nodeTree = nullptr;
    _nodeSystem = nullptr;

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
    if (!treeIdle())
        return;

    // Generate unique name
    std::string nodeTitle = _nodeTree->generateNodeName(nodeTypeID);

    NodeID nodeID = _nodeTree->createNode(nodeTypeID, nodeTitle);
    if(nodeID == InvalidNodeID)
    {
        showErrorMessage("Internal error - Node system couldn't create node of given type.");
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
    const NodeConfig* nodeConfig = _nodeTree->nodeConfigurationPtr(nodeID);
    if(!nodeConfig)
    {
        showErrorMessage("Internal error - querying node type configuration.\n"
            "Given node ID was invalid.");
        return;
    }

    QString nodeTypeName = QString::fromStdString(_nodeTree->nodeTypeName(nodeID));
    NodeView* nodeView = new NodeView(nodeTitle, nodeTypeName);
    //if(!nodeConfig->description().empty())
    //	nodeView->setToolTip(QString::fromStdString(nodeConfig->description()));

    // Add input sockets views to node view
    for(const SocketConfig& input : nodeConfig->inputs())
    {
        nodeView->addSocketView(input.socketID(), input.type(), 
                                QString::fromStdString(input.name()), false);
    }

    // Add output sockets views to node view
    for(const SocketConfig& output : nodeConfig->outputs())
    {
        nodeView->addSocketView(output.socketID(), output.type(),
                                QString::fromStdString(output.name()), true);
    }

    // Make a default property - node name
    _propManager->newPropertyGroup(nodeID, "Common");
    _propManager->newProperty(nodeID, -1, EPropertyType::String,
        "Node name", nodeTitle, QString());

    if(!nodeConfig->properties().empty())
        _propManager->newPropertyGroup(nodeID, "Specific");

    // Add rest of the properties
    for(const PropertyConfig& prop : nodeConfig->properties())
    {
        _propManager->newProperty(nodeID, prop.propertyID(), prop.type(),
            QString::fromStdString(prop.name()),
            PropertyManager::nodePropertyToVariant(_nodeTree->nodeProperty(nodeID, prop.propertyID())),
            QString::fromStdString(prop.uiHints()));
    }

    auto* propModel = _propManager->propertyModel(nodeID);
    Q_ASSERT(propModel);

    connect(propModel, &PropertyModel::propertyChanged,
        this, &Controller::changeProperty);
    connect(nodeView, &NodeView::mouseDoubleClicked,
        this, &Controller::nodeViewMouseDoubleClicked);
    connect(nodeView, &NodeView::mouseHoverEntered,
        this, &Controller::nodeViewMouseHoverEntered);
    connect(nodeView, &NodeView::mouseHoverLeft,
        this, &Controller::nodeViewMouseHoverLeft);

    // Add node view to a scene
    _nodeScene->addItem(nodeView);
    nodeView->setData(NodeDataIndex::NodeKey, nodeID);
    nodeView->setPos(scenePos);
    nodeView->setNodeWithStateMark(nodeConfig->flags().testFlag(ENodeConfig::HasState));
    _nodeViews[nodeID] = nodeView;

    // Set time info visibility on new nodes
    nodeView->setTimeInfo("0.000 ms");
    nodeView->setTimeInfoVisible(_ui->actionDisplayTimeInfo->isChecked());
}

void Controller::linkNodes(NodeID fromNodeID, SocketID fromSocketID,
        NodeID toNodeID, SocketID toSocketID)
{
    SocketAddress addrFrom(fromNodeID, fromSocketID, true);
    SocketAddress addrTo(toNodeID, toSocketID, false);

    ELinkNodesResult result = _nodeTree->linkNodes(addrFrom, addrTo);
    if(result != ELinkNodesResult::Ok)
    {
        switch(result)
        {
        case ELinkNodesResult::InvalidAddress:
            showErrorMessage("Internal error - Something was wrong with given socket addresses.\n"
                "New connection will not be made.");
            break;
        case ELinkNodesResult::CycleDetected:
            showErrorMessage("New connection would create cycle which is not permitted.\n"
                "New connection will not be made.");
            break;
        case ELinkNodesResult::TwoOutputsOnInput:
            showErrorMessage("You can't connect many-to-one outputs to an input.\n"
                "Remove first already exisiting connection.\n"
                "New connection will not be made.");
            break;
        }

        return;
    }

    Q_ASSERT(_nodeViews[fromNodeID]);
    Q_ASSERT(_nodeViews[toNodeID]);

    if(!_nodeViews[fromNodeID] || !_nodeViews[toNodeID])
    {
        showErrorMessage("Internal error - Error looking for node views with given NodeID");
        return;
    }

    linkNodesView(_nodeViews[fromNodeID]->outputSocketView(fromSocketID),
        _nodeViews[toNodeID]->inputSocketView(toSocketID));

    _nodeTreeDirty = true;
    updateTitleBar();
    processAutoRefresh();
}

void Controller::linkNodesView(NodeSocketView* from, NodeSocketView* to)
{
    Q_ASSERT(from);
    Q_ASSERT(to);

    if(!from || !to)
    {
        showErrorMessage("Internal error - One of socket is null");
        return;
    }

    // Create new link view 
    NodeLinkView* link = new NodeLinkView(from, to, nullptr);
    link->setDrawDebug(false);
    from->addLink(link);
    to->addLink(link);

    // Cache it and add it to a scene
    _linkViews.append(link);
    _nodeScene->addItem(link);
}

void Controller::duplicateNode(NodeID nodeID, const QPointF& scenePos)
{
    NodeID newNodeID = _nodeTree->duplicateNode(nodeID);
    if(newNodeID == InvalidNodeID)
    {
        showErrorMessage("Internal error - Couldn't duplicate given node.");
        return;
    }

    // Create new view associated with the model
    addNodeView(QString::fromStdString(_nodeTree->nodeName(newNodeID)), 
        newNodeID, scenePos + QPointF(40, 40));

    _nodeTreeDirty = true;
    updateTitleBar();
}

void Controller::unlinkNodes(NodeLinkView* linkView)
{
    Q_ASSERT(linkView);
    Q_ASSERT(linkView->fromSocketView());
    Q_ASSERT(linkView->toSocketView());	

    if(!linkView || !linkView->fromSocketView() || !linkView->toSocketView())
    {
        showErrorMessage("Internal error - Connection or one of its sockets are null");
        return;
    }

    const NodeSocketView* from = linkView->fromSocketView();
    const NodeSocketView* to = linkView->toSocketView();

    SocketAddress addrFrom(from->nodeView()->nodeKey(),
                           from->socketKey(), true);
    SocketAddress addrTo(to->nodeView()->nodeKey(), 
                         to->socketKey(), false);

    /// TODO: give a reason
    if(!_nodeTree->unlinkNodes(addrFrom, addrTo))
    {
        showErrorMessage("Internal error - Couldn't unlinked given sockets");
        return;
    }

    // Remove link view
    if(!_linkViews.removeOne(linkView))
    {
        showErrorMessage("Internal error - Couldn't removed link view");
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
        showErrorMessage("Internal error - Couldn't removed given node");
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

    // Restore window geometry and window state from previous session
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

    prepareRecentFileList();

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

    //if(!varGeometry.isValid() || !varState.isValid())
    //	tabifyDockWidget(_ui->propertiesDockWidget, _ui->nodesDockWidget);

#if QT_VERSION == 0x050001 || QT_VERSION == 0x050000
    /// TODO: Bug in Qt 5.0.1 : Dock widget loses its frame and bar after undocked
    _ui->nodesDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
    _ui->propertiesDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
    _ui->previewDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
    _ui->logDockWidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
#endif
    // Init menu bar and its 'view' menu
    _ui->menuView->addAction(actionNodes);
    _ui->menuView->addAction(actionProperties);
    _ui->menuView->addAction(actionPreview);
    _ui->menuView->addAction(actionLog);

    setupUiAbout();

    _ui->actionNewTree->setShortcut(QKeySequence(QKeySequence::New));
    _ui->actionOpenTree->setShortcut(QKeySequence(QKeySequence::Open));
    _ui->actionSaveTree->setShortcut(QKeySequence(QKeySequence::Save));
    _ui->actionSaveTreeAs->setShortcut(QKeySequence(QKeySequence::SaveAs));

    // Connect actions from a toolbar
    connect(_ui->actionNewTree, &QAction::triggered, this, &Controller::newTree);
    connect(_ui->actionOpenTree, &QAction::triggered, this, &Controller::openTree);
    connect(_ui->actionSaveTree, &QAction::triggered, this, &Controller::saveTree);
    connect(_ui->actionSaveTreeAs, &QAction::triggered, this, &Controller::saveTreeAs);
    
    connect(_ui->actionTagNodes, &QAction::triggered, this, &Controller::tagSelected);
    connect(_ui->actionRemoveSelected, &QAction::triggered, this, &Controller::removeSelected);

    _ui->actionTagNodes->setEnabled(false);
    _ui->actionRemoveSelected->setEnabled(false);

    connect(_ui->actionSingleStep, &QAction::triggered, this, &Controller::singleStep);
    connect(_ui->actionAutoRefresh, &QAction::triggered, this, &Controller::autoRefresh);
    connect(_ui->actionPlay, &QAction::triggered, this, &Controller::play);
    connect(_ui->actionPause, &QAction::triggered, this, &Controller::pause);
    connect(_ui->actionStop, &QAction::triggered, this, &Controller::stop);

    connect(_ui->actionDisplayTimeInfo, &QAction::triggered, this, &Controller::displayNodeTimeInfo);
    connect(_ui->actionFitToView, &QAction::triggered, this, &Controller::fitToView);
    connect(_ui->actionResetZoom, &QAction::triggered, this, &Controller::resetZoom);
    connect(_ui->actionNodesTooltips, &QAction::toggled, this, &Controller::toggleDisplayNodesTooltips);

    _ui->actionPlay->setEnabled(false);
    _ui->actionPause->setEnabled(false);
    _ui->actionStop->setEnabled(false);

    connect(_ui->graphicsView, &NodeEditorView::nodeTypeDropped, this, &Controller::addNode);

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
    _totalTimeLabel = new QLabel(this);
    _ui->statusBar->addPermanentWidget(_totalTimeLabel);
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

void Controller::setupUiAbout()
{
    QAction* actionAboutQt = new QAction(
        QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), 
        tr("About &Qt"), this);
    actionAboutQt->setToolTip(tr("Show information about Qt"));
    actionAboutQt->setMenuRole(QAction::AboutQtRole);
    connect(actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    _ui->menuHelp->addAction(actionAboutQt);

    // About OpenCV
    QAction* actionAboutOpenCV = new QAction(QIcon(":/images/opencv-logo.png"), tr("About OpenCV"), this);
    actionAboutOpenCV->setToolTip(tr("Show information about OpenCV"));
    connect(actionAboutOpenCV, &QAction::triggered, [=] {
        QString translatedTextAboutOpenCVCaption = QString(
            "<h3>About OpenCV</h3>"
            "<p>This program uses OpenCV version %1.</p>"
            ).arg(QLatin1String(CV_VERSION));
        QString translatedTextAboutOpenCVText = QString(
            "<p>OpenCV (Open Source Computer Vision Library) is an open source "
            "computer vision and machine learning software library. OpenCV was "
            "built to provide a common infrastructure for computer vision "
            "applications and to accelerate the use of machine perception in the "
            "commercial products. Being a BSD-licensed product, OpenCV makes it "
            "easy for businesses to utilize and modify the code.</p>"
            "<p>OpenCV is developed as an open source project on "
            "<a href=\"http://opencv.org/\">opencv.org</a>.</p>");
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->setWindowTitle(tr("About OpenCV"));
        msgBox->setText(translatedTextAboutOpenCVCaption);
        msgBox->setInformativeText(translatedTextAboutOpenCVText);

        QPixmap pm(QLatin1String(":/images/opencv-logo.png"));
        if (!pm.isNull())
            msgBox->setIconPixmap(pm);
        msgBox->exec();
    });
    _ui->menuHelp->addAction(actionAboutOpenCV);

    // About program
    QAction* actionAboutApplication = new QAction(tr("About ") + QApplication::applicationName(), this);
    actionAboutApplication->setToolTip(tr("Show information about ") + QApplication::applicationName());
    connect(actionAboutApplication, &QAction::triggered, [=] {
        QString translatedTextAboutCaption = QString(
            "<h3>About %1 (%2)</h3>")
            .arg(QApplication::applicationName())
            .arg(is64Bit() ? "x64" : "x86");
        // Doesn't work on gcc 4.7.4
        QString translatedTextAboutText = QString::fromWCharArray(
            L"<p>Author: Kajetan Świerk</p>"
            L"<p>This program uses icon set 'Super Mono Stickers' created by Double-J "
            L"designs <a href=\"http://www.doublejdesign.co.uk/\">www.doublejdesign.co.uk</a>, "
            L"available under a Creative Commons 3.0 Attribution license. "
            L"© 2011, Double-J designs</p>");
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->setWindowTitle(tr("About ") + QApplication::applicationName());
        msgBox->setText(translatedTextAboutCaption);
        msgBox->setInformativeText(translatedTextAboutText);

        QPixmap pm(QLatin1String(":/images/button-brick.png"));
        if (!pm.isNull())
            msgBox->setIconPixmap(pm);
        msgBox->exec();
    });
    _ui->menuHelp->addAction(actionAboutApplication);
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

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    for(int level = 0; level < tokens.count() - 1; ++level)
    {
        QString parentToken = tokens[level];
        QTreeWidgetItem* p = level == 0 
            ? findItem(parentToken)
            : findChild(parent, parentToken);
        if(!p)
        {
            p = new QTreeWidgetItem(QStringList(parentToken));
            p->setFlags(flags);

            if (parent)
            {
                // Find index of last non-childless tree item
                int index = 0;
                for (int i = 0; i < parent->childCount(); ++i)
                {
                    if (parent->child(i)->childCount() == 0)
                        break;
                    ++index;
                }

                // Insert the tree item so that all expandable items come first
                parent->insertChild(index, p);
            }

            if(level == 0)
                treeItems.append(p);
        }

        parent = p;
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, tokens.last());
    item->setData(0, Qt::UserRole, typeId);
    item->setToolTip(0, QString::fromStdString(_nodeSystem->nodeDescription(typeId)));
    item->setFlags(flags | Qt::ItemIsDragEnabled);

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
        while((i = i->parent()) != 0)
            ++level;

        // Bottom level items
        if((*iter)->childCount() == 0)
        {
            NodeTypeID typeId = (*iter)->data(0, Qt::UserRole).toUInt();
            QAction* action = new QAction(text, this);
            action->setData(typeId);

            // Level starts counting from 1 so at worst - it's gonnaa point to 0-indexed which is main menu
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

void Controller::prepareRecentFileList()
{
    _ui->menuFile->removeAction(_ui->actionQuit);

    for(int i = 0; i < MaxRecentFiles; ++i)
    {
        _actionRecentFiles[i] = new QAction(this);
        _actionRecentFiles[i]->setVisible(false);
        connect(_actionRecentFiles[i], &QAction::triggered, [=]
        {
            if(!canQuit())
                return;

            QString filePath = _actionRecentFiles[i]->data().toString();

            if(!openTreeFromFile(filePath))
            {
                int fb = QMessageBox::question(this, QApplication::applicationName(),
                    tr("\"%1\" could not be opened. Do you want to remove the reference to it from the Recent file list?")
                        .arg(filePath), QMessageBox::Yes, QMessageBox::No);
                if(fb == QMessageBox::Yes)
                {
                    // Load recent file list from the settings and remove reference to non-existant or invalid node tree
                    QSettings settings;
                    QStringList recentFiles = settings.value("recentFileList").toStringList();
                    recentFiles.removeAll(filePath);
                    settings.setValue("recentFileList", recentFiles);

                    updateRecentFileActions();
                }
            }
        });

        _ui->menuFile->addAction(_actionRecentFiles[i]);
    }

    _actionClearRecentFiles = new QAction("Clear recent file list", this);
    connect(_actionClearRecentFiles, &QAction::triggered, [=]
    {
        QSettings settings;
        settings.setValue("recentFileList", QStringList());
        updateRecentFileActions();
    });
    _ui->menuFile->addAction(_actionClearRecentFiles);

    _actionSeparatorRecentFiles = _ui->menuFile->addSeparator();
    _ui->menuFile->addAction(_ui->actionQuit);

    updateRecentFileActions();
}

void Controller::showErrorMessage(const QString& message)
{
    qCritical(qPrintable(message));
    QMessageBox::critical(nullptr, QApplication::applicationName(), message);
}

void Controller::showBadConnection(int node, int socket)
{
    const auto nodeID = static_cast<NodeID>(node);
    const auto socketID = static_cast<SocketID>(socket);

    if (!_nodeViews.contains(nodeID))
        return;
    const auto nodeView = _nodeViews[nodeID];

    for (auto& linkView : _linkViews)
    {
        if (linkView->inputConnecting(nodeView))
        {
            if (linkView->toSocketView() != nullptr &&
                linkView->toSocketView()->socketKey() == socketID)
            {
                linkView->setBad(true);
                break;
            }
        }
    }

    showErrorMessage(QString("Execution error in:\nNode: %1\n"
        "Node typename: %2\n\nError message:\n%3")
            .arg(_nodeTree->nodeName(nodeID).c_str())
            .arg(_nodeTree->nodeTypeName(nodeID).c_str())
            .arg("Wrong socket connection"));
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
            _stateLabel->setText("Processing (wait)");
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
    if(_previewSelectedNodeView != nullptr)
    {
        NodeID nodeID = _previewSelectedNodeView->nodeKey();

        QString text = QString::fromStdString(_nodeTree->nodeExecuteInformation(nodeID));
        _previewWidget->updateInformation(text);

        if(_previewSelectedNodeView->outputSocketCount() > 0)
        {
            SocketID socketID = _previewSelectedNodeView->previewSocketID();

            const NodeFlowData& outputData = _nodeTree->outputSocket(nodeID, socketID);

            if(outputData.isValid())
            {	
                switch(outputData.type())
                {
                case ENodeFlowDataType::Image:
                    _previewWidget->show(outputData.getImage());
                    return;

                case ENodeFlowDataType::ImageMono:
                    _previewWidget->show(outputData.getImageMono());
                    return;

                case ENodeFlowDataType::ImageRgb:
                    _previewWidget->show(outputData.getImageRgb());
                    return;

                default:
                    break;
                }
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

#if defined(HAVE_JAI)
    _actionDevices->setEnabled(allowed);
#endif
}

void Controller::updateTitleBar()
{
    QString tmp = _nodeTreeFilePath.isEmpty()
        ? tr("Untitled")
        : QFileInfo(_nodeTreeFilePath).fileName();
    QString windowTitle = QApplication::applicationName() + " - ";
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

    // Setup tooltip
    _nodeToolTip = new NodeToolTip(nullptr);
    _nodeToolTip->setVisible(false);
    _nodeScene->addItem(_nodeToolTip);	
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

        updateRecentFileActions(filePath);
        updateTitleBar();

        qDebug() << "Node tree successfully written to the file:" << filePath;
        return true;
    }
    else
    {
        showErrorMessage(QString("Couldn't save node tree to the output file: %1!").arg(filePath));
        return false;
    }
}

bool Controller::saveTreeToFileImpl(const QString& filePath)
{
    // Add view part to deserialized part
    json11::Json::array jsonScene;
    for (auto nvit = _nodeViews.begin(); nvit != _nodeViews.end(); ++nvit)
    {
        QPointF nodePos{_nodeViews[nvit.key()]->scenePos()};
        jsonScene.push_back(json11::Json::object{{"nodeId", nvit.key()},
                                                 {"scenePosX", nodePos.x()},
                                                 {"scenePosY", nodePos.y()}});
    }

    try
    {
        NodeTreeSerializer nodeTreeSerializer{
            QFileInfo{filePath}.absolutePath().toStdString()};
        json11::Json::object json = nodeTreeSerializer.serialize(*_nodeTree);
        json.insert(std::make_pair("scene", jsonScene)); // atach scene part

        QFile file(filePath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        // Generate JSON and pretty print it into a target file
        file.write(json11::Json{json}.pretty_print().c_str());
        return true;
    }
    catch(std::exception& ex)
    {
        logException(ex);
        showErrorMessage("Error during serializing current node tree");
        return false;
    }
}

bool Controller::openTreeFromFile(const QString& filePath)
{
    _ui->actionAutoRefresh->setChecked(false);
    createNewTree();

    bool res;
    if((res = openTreeFromFileImpl(filePath)))
    {
        _nodeTreeFilePath = filePath;
        _nodeTreeDirty = false;

        updateRecentFileActions(filePath);
        updateTitleBar();
        fitToView();

        qDebug() << "Node tree successfully opened and read from file:" << filePath;
    }

    return res;
}

bool Controller::openTreeFromFileImpl(const QString& filePath)
{
    NodeTreeSerializer nodeTreeSerializer{
        QFileInfo{filePath}.absolutePath().toStdString()};
    json11::Json json;
    
    try
    {
        json = nodeTreeSerializer.deserializeFromFile(*_nodeTree,
                                                      filePath.toStdString());
    }
    catch (std::exception& ex)
    {
        logException(ex);
        showErrorMessage(
            "Couldn't deserialized given JSON into node tree structure.\n"
            "Check logs for more details");
        return false;
    }

    // Check for warnings
    for (const auto& warn : nodeTreeSerializer.warnings())
        qWarning() << warn.c_str();

    std::string err;

    // Deserialize view part
    for (const auto& sceneElem : json["scene"].array_items())
    {
        // check schema
        if (!sceneElem.has_shape({{"nodeId", json11::Json::NUMBER},
                                  {"scenePosX", json11::Json::NUMBER},
                                  {"scenePosY", json11::Json::NUMBER}},
                                 err))
        {
            continue;
        }

        NodeID nodeID = sceneElem["nodeId"].int_value();

        NodeID mappedNodeID = get_or_default(nodeTreeSerializer.idMappings(), nodeID, 0);
        double scenePosX = sceneElem["scenePosX"].number_value();
        double scenePosY = sceneElem["scenePosY"].number_value();

        // Create new view associated with the model
        if(!_nodeViews.contains(mappedNodeID))
        {
            if(_nodeTree->nodeTypeID(mappedNodeID) != InvalidNodeTypeID)
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
        qWarning("Some scene elements were missing, adding them to scene origin. "
            "You can fix it by resaving the file");
        
        auto nodeIt = _nodeTree->createNodeIterator();
        NodeID nodeID;
        while(nodeIt->next(nodeID))
        {
            NodeID mappedNodeID = get_or_default(nodeTreeSerializer.idMappings(), nodeID, 0);
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

void Controller::updateRecentFileActions(const QString& filePath)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentFileList").toStringList();

    // if filePath is not null string we also update recent file list
    if(!filePath.isEmpty())
    {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while(recentFiles.size() > MaxRecentFiles)
            recentFiles.removeLast();
        settings.setValue("recentFileList", recentFiles);
    }

    int numRecentFiles = qMin(recentFiles.size(), (int) MaxRecentFiles);

    for(int i = 0; i < numRecentFiles; ++i)
    {
        QString text = QString("&%1 %2")
            .arg(i+1)
            .arg(QFileInfo(recentFiles[i]).fileName().replace("&", "&&"));
        _actionRecentFiles[i]->setText(text);
        _actionRecentFiles[i]->setData(recentFiles[i]);
        _actionRecentFiles[i]->setVisible(true);
    }
    for(int i = numRecentFiles; i < MaxRecentFiles; ++i)
        _actionRecentFiles[i]->setVisible(false);

    _actionSeparatorRecentFiles->setVisible(numRecentFiles > 0);
    _actionClearRecentFiles->setVisible(numRecentFiles > 0);
}

void Controller::nodeEditorContextMenu(const QPoint& globalPos, const QPointF& scenePos)
{
    if(treeIdle())
    {
        _ui->statusBar->showMessage(
            QString("Scene position: %1, %2")
                .arg(scenePos.x())
                .arg(scenePos.y()));
    }

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
        // Preview socket menu
        NodeView* nodeView = nodeViewInScene(scenePos);
        // did we clicked on a node view
        if(!nodeView)
            return;

        // Construct context menu
        QMenu menu;
        QAction actionTagNode("Tag node", nullptr);
        QAction actionRemoveNode("Remove node", nullptr);
        QAction actionDuplicateNode("Duplicate node", nullptr);
        QAction actionDisableNode("Disable node", nullptr);

        if(treeIdle())
        {
            connect(&actionTagNode, &QAction::triggered, [=] {
                _nodeTree->tagNode(nodeView->nodeKey());
            });	
            connect(&actionRemoveNode, &QAction::triggered, [=] {
                deleteNode(nodeView);
            });	
            connect(&actionDuplicateNode, &QAction::triggered, [=] {
                duplicateNode(nodeView->nodeKey(), scenePos);
            });
            connect(&actionDisableNode, &QAction::triggered, [=] {
                _nodeTree->setNodeEnable(nodeView->nodeKey(), !nodeView->isNodeEnabled());
                nodeView->setNodeEnabled(!nodeView->isNodeEnabled());
            });

            if(!nodeView->isNodeEnabled())
                actionDisableNode.setText("Enable node");

            menu.addAction(&actionTagNode);
            menu.addAction(&actionRemoveNode);
            menu.addAction(&actionDuplicateNode);
            menu.addAction(&actionDisableNode);
        }

        int previewsCount = nodeView->outputSocketCount();
        if(previewsCount > 0)
        {
            if(_state == EState::Stopped && !_processing)
                menu.addSeparator();

            int curr = nodeView->previewSocketID();
            for(int imwrite = 0; imwrite < previewsCount; ++imwrite)
            {
                QAction* actionPreviewSocket = new QAction
                    (QString("Preview socket: %1").arg(imwrite), &menu);
                actionPreviewSocket->setCheckable(true);
                actionPreviewSocket->setChecked(imwrite == curr);
                actionPreviewSocket->setData(imwrite);
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

            // Save socket image menu (disable when tree is still being processed)
            if(_state != EState::Playing && !_processing)
            {
                for(int socketID = 0; socketID < previewsCount; ++socketID)
                {
                    NodeID nodeID = nodeView->nodeKey();

                    const NodeFlowData& outputData = _nodeTree->outputSocket(nodeID, socketID);
                    if(!outputData.isValidImage())
                        continue;

                    QAction* actionSaveImageSocket = new QAction
                        (QString("Save image socket: %1").arg(socketID), &menu);
                    connect(actionSaveImageSocket, &QAction::triggered,
                        [=]
                        {
                            QString filePath = QFileDialog::getSaveFileName(
                                this, tr("Save file"), QString(), ""
                                "PNG (*.png);;"
                                "BMP (*.bmp);;"
                                "JPEG (*.jpg);;"
                                "JPEG 2000 (*.jp2)");
                            if(filePath.isEmpty())
                                return;

                            try
                            {
                                outputData.saveToDisk(filePath.toStdString());
                            }
                            catch (cv::Exception& ex)
                            {
                                showErrorMessage(QString::fromStdString(ex.msg));
                            }
                        });	
                    menu.addAction(actionSaveImageSocket);
                }
            }
        }

        // finally, show a menu
        menu.exec(globalPos);
    }
}

void Controller::nodeEditorKeyPressed(QKeyEvent* event)
{
    if(!treeIdle())
        return;

#if defined(QT_DEBUG)
    // Debugging feature
    if(event->key() == Qt::Key_1)
    {
        if(!canQuit())
            return;

        if(_actionRecentFiles[0]->isVisible())
            _actionRecentFiles[0]->trigger();
    }
#else
    Q_UNUSED(event);
#endif
}

void Controller::draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to)
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

    if(!fromSocket || !toSocket || !fromSocket->nodeView() || !toSocket->nodeView())
    {
        showErrorMessage("Internal error - One of the sockets or their node are null");
        return;
    }

    linkNodes(fromSocket->nodeView()->nodeKey(),
        fromSocket->socketKey(),
        toSocket->nodeView()->nodeKey(),
        toSocket->socketKey());
}

void Controller::nodeViewMouseDoubleClicked(NodeView* nodeView)
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

    _ui->actionTagNodes->setEnabled(!items.isEmpty()
        && std::any_of(items.begin(), items.end(), [](QGraphicsItem* item) {
            return item->type() == NodeView::Type;
        })
    );
    _ui->actionRemoveSelected->setEnabled(!items.isEmpty());

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
                    if(_nodeTree->setNodeName(nodeID, nameStd))
                    {
                        _nodeViews[nodeID]->setNodeViewName(name);
                        _nodeTreeDirty = true;
                        updateTitleBar();
                    }
                    else
                    {
                        *ok = false;
                        showErrorMessage("Node name contains some invalid characters");
                    }
                }
                else
                {
                    *ok = false;
                    showErrorMessage("Node name already taken");
                }
            }
            return;
        }
    }

    try
    {
        if(_nodeTree->nodeSetProperty(nodeID, propID, PropertyManager::variantToNodeProperty(newValue)))
        {
            _nodeTree->tagNode(nodeID);
            _nodeTreeDirty = true;
            updateTitleBar();
            processAutoRefresh();
        }
        else
        {
            *ok = false;
            qWarning(qPrintable(QString("Wrong value for nodeID: %1, propertyID: %2 (new value: %3)")
                .arg(nodeID)
                .arg(propID)
                .arg(newValue.toString())));
        }
    }
    catch(std::exception& ex)
    {
        qWarning() << ex.what();
    }
}

void Controller::newTree()
{
    if(canQuit())
    {
        createNewTree();
        _ui->graphicsView->setZoom(1.0f);
    }
}

void Controller::openTree()
{
    if(!canQuit())
        return;

    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open file"),
        _nodeTreeFilePath.isEmpty() ? QString() : QFileInfo(_nodeTreeFilePath).absolutePath(),
        "Node tree files (*.tree)");
    if(filePath.isEmpty())
        return;

    openTreeFromFile(filePath);
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
        this, tr("Save file as..."),
        QFileInfo(_nodeTreeFilePath).absolutePath(),
        "Node tree files (*.tree)"); 
    if(filePath.isEmpty())
        return false;

    return saveTreeToFile(filePath);
}

void Controller::removeSelected()
{
    if(!treeIdle())
        return;

    QList<QGraphicsItem*> selectedItems = _nodeScene->selectedItems();
    if(selectedItems.isEmpty())
        return;

    //int ret = QMessageBox::question(this, applicationTitle, 
    //	"Are you sure to remove all selected items?", 
    //	QMessageBox::Yes, QMessageBox::Cancel);
    //if(ret != QMessageBox::Yes)
    //	return;

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
}

void Controller::tagSelected()
{
    if(!treeIdle())
        return;

    QList<QGraphicsItem*> selectedItems = _nodeScene->selectedItems();

    for(auto item : selectedItems)
    {
        if(item->type() == NodeView::Type)
        {
            auto nodeView = static_cast<NodeView*>(item);
            _nodeTree->tagNode(nodeView->nodeKey());
        }
    }	
}

void Controller::updatePreview(bool res)
{
    // Worker reported something gone wrong
    if(!res)
    {
        _processing = false;
        if(!_videoMode)
            setInteractive(true);
        else
            stop();
        return;
    }

    auto execList = _nodeTree->executeList();

    // Update preview window if necessary
    if(shouldUpdatePreview(execList))
        updatePreviewImpl();

    // Job is done - enable editing
    _processing = false;

    // Update time info on nodes
    double totalTimeElapsed = 0.0;
    for(auto nodeID : execList)
    {
        using namespace std::chrono;
        long long totalMicroseconds =
            duration_cast<microseconds>(_nodeTree->nodeTimeElapsed(nodeID)).count();
        const auto elapsed =
            static_cast<double>((totalMicroseconds / 1000) + (totalMicroseconds % 1000) * 1e-3);
        totalTimeElapsed += elapsed;
        QString text = QString::number(elapsed, 'f', 3);
        text += QStringLiteral(" ms");
        _nodeViews[nodeID]->setTimeInfo(text);
    }

    _totalTimeLabel->setText(QString("Total time: %1 ms |")
        .arg(QString().setNum(totalTimeElapsed, 'f', 3)));

    if(!_videoMode)
    {
        setInteractive(true);
        updateState(EState::Stopped);
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
        updateState(EState::Stopped);
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

    QRectF sceneRect;
    for(auto* item : items)
        sceneRect = sceneRect.united(item->sceneBoundingRect());
    _ui->graphicsView->fitInView(sceneRect, Qt::KeepAspectRatio);

    // We assume there's no rotation
    QTransform t = _ui->graphicsView->transform();
    
    _ui->graphicsView->setZoom(t.m11() * 0.90);
}

void Controller::resetZoom()
{
    _ui->graphicsView->setZoom(1.0f);
}

void Controller::toggleDisplayNodesTooltips(bool checked)
{
    _showTooltips = checked;

    if(!_showTooltips && _nodeToolTip)
    {
        // Hide shown tooltip
        _nodeToolTip->setVisible(false);
    }
    else if(_showTooltips && _nodeToolTip)
    {
        // Show tooltip if already over node view
        QPoint cursorWidgetPos = _ui->graphicsView->mapFromGlobal(QCursor::pos());
        QPointF scenePos = _ui->graphicsView->mapToScene(cursorWidgetPos);

        // Preview socket menu
        NodeView* nodeView = nodeViewInScene(scenePos);
        if(!nodeView)
            return;

        auto text = _nodeTree->nodeExecuteInformation(nodeView->nodeKey());
        if(!text.empty())
        {
            _nodeToolTip->setText(QString::fromStdString(text));
            _nodeToolTip->setPos(scenePos);
            _nodeToolTip->setVisible(true);
        }
    }
}

NodeView* Controller::nodeViewInScene(const QPointF& scenePos)
{
    QList<QGraphicsItem*> items = _nodeScene->items(scenePos, 
        Qt::ContainsItemShape, Qt::AscendingOrder);

    // If the user clicked onto empty space
    if(items.isEmpty())
        return nullptr;

    // Preview socket menu
    NodeView* nodeView = nullptr;

    for(int i = 0; i < items.size(); ++i)
    {
        auto item = items[i];

        if(item->type() != NodeView::Type)
            continue;
        nodeView = static_cast<NodeView*>(item);
    }

    // Can be nullptr here!
    return nodeView;
}

void Controller::nodeViewMouseHoverEntered(NodeID nodeID, QGraphicsSceneHoverEvent* event)
{
    if(_showTooltips && _nodeToolTip != nullptr)
    {
        auto text = _nodeTree->nodeExecuteInformation(nodeID);
        if(!text.empty())
        {
            _nodeToolTip->setText(QString::fromStdString(text));
            _nodeToolTip->setPos(event->scenePos());
            _nodeToolTip->setVisible(true);
        }
    }
}

void Controller::nodeViewMouseHoverLeft(NodeID nodeID, QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(nodeID);
    Q_UNUSED(event);
    if(_nodeToolTip != nullptr)
        _nodeToolTip->setVisible(false);
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

/// TODO: Move to separate file

void Controller::initGpuModule(QMenu* menuModules)
{
    if(!_gpuModule)
        return;

    _nodeSystem->registerNodeModule(_gpuModule);
    _gpuModule->onCreateInteractive = 
        [=](const std::vector<GpuPlatform>& gpuPlatforms) {
            GpuChoiceDialog dialog(gpuPlatforms);
            dialog.exec();

            GpuInteractiveResult result = { 
                dialog.result() == QDialog::Accepted
                ? dialog.deviceType()
                : EDeviceType::None,
                dialog.platformID(), dialog.deviceID() };
            return result;
        };

    _actionInteractiveSetup = new QAction(QStringLiteral("Init module"), this);
    _actionListPrograms = new QAction(QStringLiteral("List programs"), this);

    connect(_actionListPrograms, &QAction::triggered,
        this, &Controller::showProgramsList);
    connect(_actionInteractiveSetup, &QAction::triggered,
        [=]
    {
        if(_gpuModule->isInitialized())
        {
            QMessageBox::warning(nullptr, QApplication::applicationName(),
                "GPU Module - GPU Module already initialized");
            _actionInteractiveSetup->setEnabled(false);
            return;
        }

        const auto& gpuPlatforms = _gpuModule->availablePlatforms();
        if(gpuPlatforms.empty())
        {
            showErrorMessage("GPU Module - No OpenCL platforms found!");
            return;
        }

        bool oldv = _gpuModule->isInteractiveInit();
        _gpuModule->setInteractiveInit(true);
        if(_gpuModule->initialize())
        {
            _actionInteractiveSetup->setEnabled(false);
            _gpuModule->setInteractiveInit(oldv);
        }			
    });

    auto menuGpu = menuModules->addMenu("GPU");
    menuGpu->addAction(_actionInteractiveSetup);
    menuGpu->addAction(_actionListPrograms);
}

void Controller::showProgramsList()
{
    auto list = _gpuModule->populateListOfRegisteredPrograms();
    
    QList<QTreeWidgetItem*> items;
    for(const auto& programEntry : list)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem((QTreeWidget*)nullptr, 
            QStringList() << QString::fromStdString(programEntry.programName));

        for(const auto& build : programEntry.builds)
        {
            QString buildOptions = QString::fromStdString(build.options);
            if(buildOptions.isEmpty())
                buildOptions = QStringLiteral("[no build options]");
            QTreeWidgetItem* buildItem = new QTreeWidgetItem(item, QStringList() << buildOptions);
            
            for(const auto& kernel : build.kernels)
            {
                QString kernelName = QString::fromStdString(kernel);
                QTreeWidgetItem* kernelItem = new QTreeWidgetItem(buildItem, QStringList() << kernelName);
                Q_UNUSED(kernelItem);
            }
        }

        items.append(item);
    }

    QDialog dialog;
    dialog.resize(500,400);
    QHBoxLayout* horizontalLayout = new QHBoxLayout(&dialog);

    QTreeWidget* treeWidget = new QTreeWidget(&dialog);
    treeWidget->setColumnCount(1);
    treeWidget->insertTopLevelItems(0, items);
    treeWidget->headerItem()->setText(0, QStringLiteral("Registered programs:"));
    treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeWidget->expandAll();

    horizontalLayout->addWidget(treeWidget);

    QAction* actionRebuildProgram = new QAction("Rebuild", &dialog);
    connect(actionRebuildProgram, &QAction::triggered, [&] {
        QModelIndex index = treeWidget->currentIndex();
        int r = index.row();
        if((size_t) r < list.size())
        {
            std::string programName = list[r].programName;

            QProgressDialog	progress;
            progress.setLabel(new QLabel(QString("Rebuilding program %1...")
                .arg(QString::fromStdString(programName)), &progress));
            progress.setCancelButton(nullptr);
            progress.setRange(0, 0);
            progress.setWindowModality(Qt::ApplicationModal);

            std::exception_ptr exception;
            std::thread buildThread([&] {
                try
                {
                    // Will throw if program build won't succeed
                    _gpuModule->rebuildProgram(programName);

                    // go through all gpu nodes (could be somehow filtered later) and tag them
                    auto nit = _nodeTree->createNodeIterator();
                    NodeID nodeID;
                    while(nit->next(nodeID))
                    {
                        const NodeConfig& nodeConfig = _nodeTree->nodeConfiguration(nodeID);
                        if(nodeConfig.module() == "opencl")
                            _nodeTree->tagNode(nodeID);
                    }
                }
                catch (...)
                {
                    // Exception propagate to main thread
                    exception = std::current_exception();
                }
                QMetaObject::invokeMethod(&progress, "close", Qt::QueuedConnection);				
            });

            progress.exec();
            buildThread.join(); // when user forced progress dialog to be closed

            try
            {
                if(exception != nullptr)
                    std::rethrow_exception(exception);
            }
            catch (GpuBuildException& ex)
            {
                showErrorMessage(QString::fromStdString(ex.formatted));
            }
        }
    });

    connect(treeWidget, &QTreeWidget::itemSelectionChanged, [&] {
        QModelIndex index = treeWidget->currentIndex();
        QModelIndex parent;
        if((parent = index.parent()).isValid())
            treeWidget->removeAction(actionRebuildProgram);
        else
            treeWidget->addAction(actionRebuildProgram);			
    });
    treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

    dialog.exec();
}

/// TODO: Move to separate file
#include "ui_Camera.h"

void Controller::initJaiModule(QMenu* menuModules)
{
    if(!_jaiModule)
        return;

    _nodeSystem->registerNodeModule(_jaiModule);

    _actionInitModule = new QAction(QStringLiteral("Initialize"), this);
    _actionDevices = new QAction(QStringLiteral("Devices"), this);

    auto menuJai = menuModules->addMenu("JAI");
    menuJai->addAction(_actionDevices);

    connect(_actionDevices, &QAction::triggered, 
        this, &Controller::showDeviceSettings);
}

void populateWidget(QLineEdit* lineEdit, const RangedValue<int64_t>& rv)
{
    lineEdit->setText(QString::number(rv.value));
    lineEdit->setValidator(new QIntValidator(rv.minValue, rv.maxValue, lineEdit));
    lineEdit->setToolTip(QString("[%1..%2]")
        .arg(rv.minValue)
        .arg(rv.maxValue));
}

void modifyLabels(QLabel* label, const RangedValue<int64_t>& rv)
{
    label->setText(label->text() + QString(" [%1..%2]").arg(rv.minValue).arg(rv.maxValue));
}

void queryAndFillParameters(IJaiNodeModule& jaiModule,
                            Ui::CameraDialog& ui, 
                            const CameraInfo& info, 
                            int index)
{
    ui.cameraIdLineEdit->setText(QString::fromStdString(info.id));
    ui.manufacturerLineEdit->setText(QString::fromStdString(info.manufacturer));
    ui.interfaceLineEdit->setText(QString::fromStdString(info.interfaceId));
    ui.ipAddressLineEdit->setText(QString::fromStdString(info.ipAddress));
    ui.macAddressLineEdit->setText(QString::fromStdString(info.macAddress));

    CameraSettings settings = jaiModule.cameraSettings(index);

    populateWidget(ui.offsetXLineEdit, settings.offsetX);
    populateWidget(ui.offsetYLineEdit, settings.offsetY);
    populateWidget(ui.widthLineEdit, settings.width);
    populateWidget(ui.heightLineEdit, settings.height);
    populateWidget(ui.adcGainLineEdit, settings.gain);

    modifyLabels(ui.offsetXLabel, settings.offsetX);
    modifyLabels(ui.offsetYLabel, settings.offsetY);
    modifyLabels(ui.widthLabel, settings.width);
    modifyLabels(ui.heightLabel, settings.height);
    modifyLabels(ui.adcGainLabel, settings.gain);

    int pindex = 0;
    for(const auto& px : settings.pixelFormats)
    {
        int pxcode = std::get<0>(px);
        ui.pixelFormatComboBox->addItem(QString::fromStdString(std::get<1>(px)), pxcode);
        if(pxcode == settings.pixelFormat)
            ui.pixelFormatComboBox->setCurrentIndex(pindex);
        ++pindex;
    }
}

void Controller::showDeviceSettings()
{
    QProgressDialog	progress;
    progress.setLabel(new QLabel("Initializing JAI module...", &progress));
    progress.setCancelButton(nullptr);
    progress.setRange(0, 0);
    progress.setWindowModality(Qt::ApplicationModal);

    if(!_jaiModule->isInitialized())
    {
        std::future<bool> res = std::async(std::launch::async, [&] {
            bool res = _jaiModule->initialize();
            // Situation when close() is called before progress.exec() 
            // is not possible as it goes to its own EventLoop 
            // Or at least it think that's the case
            QMetaObject::invokeMethod(&progress, "close", Qt::QueuedConnection);
            return res;
        });
        progress.exec();

        if(!res.get())
        {
            showErrorMessage("JAI Module - Couldn't initialized JAI module!");
            return;
        }
    }

    if(_jaiModule->cameraCount() == 0)
    {
        showErrorMessage("JAI Module - No camera has been found.\nCheck your connection and try again");
        return;
    }

    std::vector<CameraInfo> cameraInfos = _jaiModule->discoverCameras(EDriverType::All);
    if(cameraInfos.empty())
    {
        showErrorMessage("JAI Module - Couldn't detect any camera devices");
        return;
    }

    QDialog dialog;
    Ui::CameraDialog ui;
    ui.setupUi(&dialog);

    for(const auto& info : cameraInfos)
        ui.cameraComboBox->addItem(QString::fromStdString(info.modelName));
    queryAndFillParameters(*_jaiModule, ui, cameraInfos[0], 0);

    connect(ui.cameraComboBox, static_cast<void (QComboBox::*)(int)>
        (&QComboBox::currentIndexChanged), [&](int index) {
        ui.offsetXLabel->setText("Offset X");
        ui.offsetYLabel->setText("Offset Y");
        ui.widthLabel->setText("Width");
        ui.heightLabel->setText("Height");
        ui.adcGainLabel->setText("ADC Gain");
        ui.pixelFormatLabel->setText("Pixel format");
        ui.pixelFormatComboBox->clear();
        queryAndFillParameters(*_jaiModule, ui, cameraInfos[index], index);
    });

    connect(ui.buttonBox, &QDialogButtonBox::accepted, [&] {
        int index = ui.cameraComboBox->currentIndex();
        CameraSettings settings;
        settings.offsetX.value = ui.offsetXLineEdit->text().toInt();
        settings.offsetY.value = ui.offsetYLineEdit->text().toInt();
        settings.width.value = ui.widthLineEdit->text().toInt();
        settings.height.value = ui.heightLineEdit->text().toInt();
        settings.gain.value = ui.adcGainLineEdit->text().toInt();
        settings.pixelFormat = ui.pixelFormatComboBox->itemData(
            ui.pixelFormatComboBox->currentIndex()).toInt();

        if(!_jaiModule->setCameraSettings(index, settings))
            showErrorMessage("JAI Module - Error occured during setting given camera's parameters");
        else
            dialog.accept();
    });

    dialog.exec();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

#if K_SYSTEM == K_SYSTEM_WINDOWS
static const QString pluginExtensionName = QStringLiteral("*.dll");
#elif K_SYSTEM == K_SYSTEM_LINUX
static const QString pluginExtensionName = QStringLiteral("*.so");
#endif

static QString absolutePathToChildDirectory(const QString& absFilePath,
                                            const char* childDirectoryName)
{
    QFileInfo fi(absFilePath);
    QDir dir = fi.absoluteDir();
    dir.cd(childDirectoryName); // don't mind error here
    return dir.absolutePath();
}

static QString pluginDirectory()
{
    auto execPath = executablePath();
    return absolutePathToChildDirectory(
        QString::fromUtf8(execPath.c_str(), execPath.size()), "plugins");
}

}

void Controller::pluginLookUp()
{
    QDir pluginDir(pluginDirectory());
    QList<QFileInfo> list = pluginDir.entryInfoList(
        QStringList(pluginExtensionName), QDir::Files);

    for(const auto& pluginName : list)
    {
        QString pluginBaseName = pluginName.completeBaseName();
        QString pluginPath = pluginName.absoluteFilePath();
        try
        {
            size_t typesRegisted = _nodeSystem->loadPlugin(pluginPath.toStdString());
            qDebug() << "Plugin" << pluginBaseName << "loaded successfully -" << typesRegisted
                     << "node type(s) registered.";
        }
        catch (std::exception& ex)
        {
            // Silent error
            qCritical() << "Couldn't load plugin" << pluginBaseName << "- details:" << ex.what();
        }
    }
}