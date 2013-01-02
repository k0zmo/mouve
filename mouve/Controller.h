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

private slots:
	void draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStart(QGraphicsWidget* from);
	void draggingLinkStop(QGraphicsWidget* from);

	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

	void executeClicked();
	void mouseDoubleClickNodeView(NodeView* nodeView);

private:
	QList<NodeLinkView*> _linkViews;
	QHash<NodeID, NodeView*> _nodeViews;

	NodeView* _previewSelectedNodeView;

	NodeScene* _nodeScene;

	std::unique_ptr<NodeSystem> _nodeSystem;
	std::unique_ptr<NodeTree> _nodeTree;
	/// xXx: QList< std::unique_ptr<NodeTree> > _nodeTrees;

	QList<QAction*> _addNodesActions;

	Ui::MainWindow* _ui;

private:
	void updatePreview();
};

#define gC Controller::instancePtr()
