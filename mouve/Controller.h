#pragma once

#include "Prerequisites.h"
#include "Singleton.h"

#include <QMainWindow>

namespace Ui {
	class MainWindow;
}

class Controller
	: public QMainWindow
	, public Singleton<Controller>
{
	Q_OBJECT    
public:
	explicit Controller(QWidget* parent = nullptr, Qt::WFlags flags = 0);
	virtual ~Controller();

private:
	void addNode(NodeTypeID nodeTypeID, const QPointF& scenePos);
	void addNodeView(const QString& nodeTitle,
		NodeID nodeID, const QPointF& scenePos);

	void linkNodeViews(NodeSocketView* from, NodeSocketView* to);
	void unlinkNodeViews(NodeLinkView* linkView);

	void deleteNodeView(NodeView* nodeView);

	void refreshSelectedNode(NodeView* nodeView);

private slots:
	void draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStarted(QGraphicsWidget* from);
	void draggingLinkStopped(QGraphicsWidget* from);
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

	void executeClicked();
	void nodeSceneSelectionChanged();

private:
	QList<NodeLinkView*> _linkViews;
	QHash<NodeID, NodeView*> _nodeViews;

	NodeScene* _nodeScene;

	std::unique_ptr<NodeSystem> _nodeSystem;
	std::unique_ptr<NodeTree> _nodeTree;
	/// xXx: QList< std::unique_ptr<NodeTree> > _nodeTrees;

	QList<QAction*> _addNodesActions;

	Ui::MainWindow* _ui;
};

#define gC Controller::instancePtr()
