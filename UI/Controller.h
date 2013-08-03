#pragma once

#include "Prerequisites.h"
#include "Kommon/Singleton.h"

#include <QMainWindow>
#include <QThread>

namespace Ui {
class MainWindow;
}
class QGraphicsScene;
class QGraphicsWidget;
class QLabel;
class TreeWorker;
class QProgressBar;
class QTreeWidgetItem;
class QGraphicsSceneHoverEvent;

class IJaiNodeModule;
class IGpuNodeModule;

enum class EState
{
	Stopped,
	Playing,
	Paused
};

class Controller
	: public QMainWindow
	, public Singleton<Controller>
{
	Q_OBJECT
public:
	explicit Controller(QWidget* parent = nullptr, 
		Qt::WindowFlags flags = 0);
	~Controller() override;

	void addNode(NodeTypeID nodeTypeID, const QPointF& scenePos);
	void linkNodes(NodeID fromNodeID, SocketID fromSocketID,
		NodeID toNodeID, SocketID toSocketID);
	void duplicateNode(NodeID nodeID, const QPointF& scenePos);

protected:
	void closeEvent(QCloseEvent* event);

private:
	void addNodeView(const QString& nodeTitle,
		NodeID nodeID, const QPointF& scenePos);
	void linkNodesView(NodeSocketView* from, NodeSocketView* to);

	/// TODO: Split to unlinkNodes and unlinkNodesView Called from the controller itself
	// Called from the controller itself
	void unlinkNodes(NodeLinkView* linkView);
	void deleteNode(NodeView* nodeView);

	void setupUi();

	void prepareRecentFileList();

	void setupUiAbout();
	void setupNodeTypesUi();
	void addNodeTypeTreeItem(NodeTypeID typeId,
		const QStringList& tokens, QList<QTreeWidgetItem*>& treeItems);
	void populateAddNodeContextMenu();

	// Enables (disables) video or image control toolbar buttons
	void switchToVideoMode();
	void switchToImageMode();

	void updateState(EState state);
	void updateControlButtonState(EState state);

	// Auto-refresh functionality
	bool isAutoRefresh();
	void processAutoRefresh();

	void queueProcessing(bool withInit = false);
	bool shouldUpdatePreview(const std::vector<NodeID>& executedNodes);
	void updatePreviewImpl();
	void setInteractive(bool allowed);

	// Updates window title whenever current node tree changes
	// (its name, dirty status and so on)
	void updateTitleBar();

	// If changes are not saved shows adequate message 
	// Returns false if user "changed his mind"
	bool canQuit();

	// Returns true if there isn't any job pending or being processed
	bool treeIdle();

	// Creating new node tree
	void clearTreeView();
	void createNewNodeScene();
	void createNewTree();

	// Saving node tree to a given file (json serialization)
	bool saveTreeToFile(const QString& filePath);
	bool saveTreeToFileImpl(const QString& filePath);

	// Opening node tree from a given file (json deserialization)
	bool openTreeFromFile(const QString& filePath);
	bool openTreeFromFileImpl(const QString& filePath);

	void updateRecentFileActions(const QString& filePath = QString());

	void pluginLookUp();

	NodeView* nodeViewInScene(const QPointF& scenePos);

private slots:
	void showErrorMessage(const QString& message);

	// Slots for QGraphicsView's events
	void nodeEditorContextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void nodeEditorKeyPressed(QKeyEvent* event);

	void draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to);

	// Slots for NodeViews' events
	void nodeViewMouseDoubleClicked(NodeView* nodeView);
	void nodeViewMouseHoverEntered(NodeID nodeID, QGraphicsSceneHoverEvent* event);
	void nodeViewMouseHoverLeft(NodeID nodeID, QGraphicsSceneHoverEvent* event);

	// Updates property view with new property model
	void sceneSelectionChanged();

	// Slot for UI part of Property::setData 
	void changeProperty(NodeID nodeID, PropertyID propID, 
		const QVariant& newValue, bool* ok);

	// Toggles displaying nodes' time info
	void displayNodeTimeInfo(bool checked);

	// Called after node tree execution has been finished
	void updatePreview(bool res);

	// Menu (toolbar) actions
	void newTree();
	void openTree();
	bool saveTree();
	bool saveTreeAs();

	// Edit menu
	void removeSelected();
	void tagSelected();

	// Image mode toolbar
	void singleStep();
	void autoRefresh();

	// Video mode toolbar
	void play();
	void pause();
	void stop();

	// Fit whole scene to a current view size
	void fitToView();

	// Node execute information as a tooltip
	void toggleDisplayNodesTooltips(bool checked);

private:
	/// TODO: QHash
	QList<NodeLinkView*> _linkViews;
	QHash<NodeID, NodeView*> _nodeViews;

	NodeView* _previewSelectedNodeView;
	NodeToolTip* _nodeToolTip;
	QGraphicsScene* _nodeScene;
	PropertyManager* _propManager;

	std::unique_ptr<NodeSystem> _nodeSystem;
	std::shared_ptr<NodeTree> _nodeTree;

	QThread _workerThread;
	TreeWorker* _treeWorker;
	QProgressBar* _progressBar;

	// Recent files part
	enum { MaxRecentFiles = 15 };
	QAction* _actionRecentFiles[MaxRecentFiles];
	QAction* _actionSeparatorRecentFiles;
	QAction* _actionClearRecentFiles;

	//
	// OpenCL Module
	//
	std::shared_ptr<IGpuNodeModule> _gpuModule;
	QAction* _actionListPrograms;
	QAction* _actionInteractiveSetup;

	void showProgramsList();
	void initGpuModule(QMenu* menuModules);

	//
	// JAI Module
	//
	std::shared_ptr<IJaiNodeModule> _jaiModule;
	QAction* _actionInitModule;
	QAction* _actionDevices;

	void showDeviceSettings();
	void initJaiModule(QMenu* menuModules);

	//
	// UI part
	//
	Ui::MainWindow* _ui;
	PreviewWidget* _previewWidget;
	QMenu* _contextMenuAddNodes;

	QLabel* _totalTimeLabel;
	QLabel* _stateLabel;
	EState _state;
	bool _processing;
	bool _videoMode;
	bool _startWithInit;

	QString _nodeTreeFilePath;
	bool _nodeTreeDirty;

	bool _showTooltips;
};

#define gC Controller::instancePtr()
