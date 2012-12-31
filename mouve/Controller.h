#pragma once

#include "Prerequisites.h"
#include "Singleton.h"

// UI
#include "ui_MainWindow.h"

//static bool DEBUG_LINKS = false;

class Controller
	: public QMainWindow
	, public Singleton<Controller>
	, private Ui::MainWindow
{
	Q_OBJECT    
public:
	explicit Controller(QWidget* parent = nullptr, Qt::WFlags flags = 0);
	virtual ~Controller();

private:
	void addNode(NodeTypeID nodeTypeID, const QPointF& scenePos);
	void addNodeView(const QString& nodeTitle,
		NodeID nodeID, const QPointF& scenePos);

private slots:
	void draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStarted(QGraphicsWidget* from);
	void draggingLinkStopped(QGraphicsWidget* from);
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

private:
	QList<NodeLinkView*> mLinkViews;
	QHash<NodeID, NodeView*> mNodeViews;

	NodeScene* mNodeScene;

	std::unique_ptr<NodeSystem> mNodeSystem;
	std::unique_ptr<NodeTree> mNodeTree;
	/// xXx: QList< std::unique_ptr<NodeTree> > mNodeTrees;

	QList<QAction*> mAddNodesActions;
};

#define gC Controller::instancePtr()
