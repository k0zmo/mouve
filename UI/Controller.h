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
	explicit Controller(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
	virtual ~Controller();

private:
	void addNode(NodeTypeID nodeTypeID, const QPointF& scenePos);
	void addNodeView(const QString& nodeTitle,
		NodeID nodeID, const QPointF& scenePos);

	void linkNodes(NodeSocketView* from, NodeSocketView* to);
	void unlinkNodes(NodeLinkView* linkView);

	void deleteNode(NodeView* nodeView);

	/// NEW
	void setupUi();
	bool isAutoRefresh();

	void switchToVideoMode();
	void switchToImageMode();

private slots:
	void draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStart(QGraphicsWidget* from);
	void draggingLinkStop(QGraphicsWidget* from);

	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

	void mouseDoubleClickNodeView(NodeView* nodeView);

	/// NEW
	void sceneSelectionChanged();
	void changeProperty(NodeID nodeID, PropertyID propID, 
		const QVariant& newValue, bool* ok);

	void singleStep();
	void autoRefresh();

	void play();
	void pause();
	void stop();

private:
	/// TODO: QHash
	QList<NodeLinkView*> _linkViews;
	QHash<NodeID, NodeView*> _nodeViews;

	NodeView* _previewSelectedNodeView;
	NodeScene* _nodeScene;
	PropertyManager* _propManager;

	std::unique_ptr<NodeSystem> _nodeSystem;
	std::unique_ptr<NodeTree> _nodeTree;

	// Temporary
	QList<QAction*> _addNodesActions;
	QTimer* _videoTimer;

	Ui::MainWindow* _ui;

	bool _videoMode;
	bool _currentlyPlaying;
	bool _startWithInit;

private:
	void showErrorMessage(const QString& message);

	/// NEW
	void processAutoRefresh();
	bool shouldUpdatePreview(const std::vector<NodeID>& executedNodes);
	void updatePreview();
	void setInteractive(bool allowed);
};

#define gC Controller::instancePtr()
