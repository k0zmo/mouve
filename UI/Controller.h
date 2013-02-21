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

private slots:
	void draggingLinkDrop(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStart(QGraphicsWidget* from);
	void draggingLinkStop(QGraphicsWidget* from);

	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

	void executeClicked();
	void mouseDoubleClickNodeView(NodeView* nodeView);

	/// NEW
	void sceneSelectionChanged();

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
	void showErrorMessage(const QString& message);
	void updatePreview(const std::vector<NodeID>& executedNodes);
};

#define gC Controller::instancePtr()
