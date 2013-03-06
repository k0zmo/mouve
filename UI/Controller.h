#pragma once

#include "Prerequisites.h"
#include "Common/Singleton.h"

#include <QMainWindow>
#include <QGraphicsWidget>

namespace Ui {
	class MainWindow;
}

class Controller
	: public QMainWindow
	, public Singleton<Controller>
{
	Q_OBJECT
public:
	explicit Controller(QWidget* parent = nullptr, 
		Qt::WindowFlags flags = 0);
	~Controller() override;

protected:
	void closeEvent(QCloseEvent* event);

private:
	void addNode(NodeTypeID nodeTypeID, const QPointF& scenePos);
	void addNodeView(const QString& nodeTitle,
		NodeID nodeID, const QPointF& scenePos);

	void linkNodes(NodeID fromNodeID, SocketID fromSocketID,
		NodeID toNodeID, SocketID toSocketID);
	void linkNodesView(NodeSocketView* from, NodeSocketView* to);

	// Called from the controller itself
	void unlinkNodes(NodeLinkView* linkView);
	void deleteNode(NodeView* nodeView);

	void setupUi();
	void showErrorMessage(const QString& message);

	// Enables (disables) video or image control toolbar buttons
	void switchToVideoMode();
	void switchToImageMode();

	// Auto-refresh functionality
	bool isAutoRefresh();
	void processAutoRefresh();

	bool shouldUpdatePreview(const std::vector<NodeID>& executedNodes);
	void updatePreviewImpl();
	void setInteractive(bool allowed);

	// Updates window title whenever current node tree changes
	// (its name, dirty status and so on)
	void updateTitleBar();

	// If changes are not saved shows adequate message 
	// Returns false if user "changed his mind"
	bool canQuit();

	// Creating new node tree
	void clearTreeView();
	void createNewNodeScene();
	void createNewTree();

	// Saving node tree to a given file (json serialization)
	bool saveTreeToFile(const QString& filePath);
	bool saveTreeToFileImpl(const QString& filePath);

	// Opening node tree from a given file (json deserialization)
	bool openTreeFromFileImpl(const QString& filePath);
	void loadNodes(const QVariantList& nodes, QMap<uint, NodeID>& oldToNewNodeID);
	void loadLinks(const QVariantList& links, const QMap<uint, NodeID>& oldToNewNodeID);

private slots:
	/// Temporary
	void draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to);
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);
	void mouseDoubleClickNodeView(NodeView* nodeView);

	// Called after node tree execution has been finished
	void updatePreview();

	void sceneSelectionChanged();
	void changeProperty(NodeID nodeID, PropertyID propID, 
		const QVariant& newValue, bool* ok);

	// Menu (toolbar) actions
	void newTree();
	void openTree();
	bool saveTree();
	bool saveTreeAs();

	// Image mode toolbar
	void singleStep();
	void autoRefresh();

	// Video mode toolbar
	void play();
	void pause();
	void stop();

	// Fit whole scene to a current view size
	void fitToView();

private:
	/// TODO: QHash
	QList<NodeLinkView*> _linkViews;
	QHash<NodeID, NodeView*> _nodeViews;

	NodeView* _previewSelectedNodeView;
	QGraphicsScene* _nodeScene;
	PropertyManager* _propManager;

	std::unique_ptr<NodeSystem> _nodeSystem;
	std::shared_ptr<NodeTree> _nodeTree;

	// Temporary
	QList<QAction*> _addNodesActions;
	QTimer* _videoTimer;

	Ui::MainWindow* _ui;
	PreviewWidget* _previewWidget;

	bool _videoMode;
	bool _currentlyPlaying;
	bool _startWithInit;

	QString _nodeTreeFilePath;
	bool _nodeTreeDirty;
};

#define gC Controller::instancePtr()
