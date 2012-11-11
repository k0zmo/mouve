#pragma once

#include "Prerequisites.h"

#include "Singleton.h"
#include "NodeView.h"
#include "NodeLinkView.h"

#include <QHash>

static bool DEBUG_LINKS = false;

class NodeController : 
	public QObject,
	public Singleton<NodeController>	
{
	Q_OBJECT
public:
	NodeController(QObject* parent = nullptr);
	~NodeController();

	// Zwraca widok wezla o podanym kluczu ID
	NodeView* nodeView(quint32 nodeKey);
	// Dodaje nowy widok wezla
	NodeView* addNodeView(quint32 nodeKey, const QString& title);
	// Usuwa widok wezla o podanym kluczu ID
	bool deleteNodeView(quint32 nodeKey);
	bool deleteNodeView(NodeView* nodeView);

	// Tworzy nowe (wizualne) polaczenie miedzy danymi gniazdami
	void linkNodeViews(NodeSocketView* from, NodeSocketView* to);

	// Usuwa polaczenie (wizualne) miedzy danymi gniazdami
	bool unlinkNodeViews(NodeSocketView* from, NodeSocketView* to);
	bool unlinkNodeViews(NodeLinkView* link);

	void addNode(quint32 nodeTypeId, const QPointF& scenePos);
	void initSampleScene();

public slots:
	void draggingLinkDropped(QGraphicsWidget* from, QGraphicsWidget* to);
	void draggingLinkStarted(QGraphicsWidget* from);
	void draggingLinkStopped(QGraphicsWidget* from);
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);

private:
	QList<NodeLinkView*> mLinkViews;
	QHash<quint32, NodeView*> mNodeViews;

	NodeScene* mScene;
	NodeEditorView* mView;

	QList<QAction*> mAddNodesActions;
};

#define nC NodeController::instancePtr()