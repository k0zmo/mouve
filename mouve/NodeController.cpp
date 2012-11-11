#include "NodeController.h"
#include "NodeScene.h"
#include "NodeEditorView.h"
#include "NodeSocketView.h"

#include <QMenu>
#include <QAction> 
#include <QDebug>
#include <QMessageBox>

static QString registeredNodeTypes[] = {
	"Image from disk",
	"Gaussian blur",
	"Preview window",
	"Grayscale",
	"Mixture of Gaussians"
};
static int nodeId = 0;

template<> NodeController* Singleton<NodeController>::singleton = nullptr;

NodeController::NodeController(QObject* parent)
	: QObject(parent)
	, mScene(new NodeScene(this))
	, mView(new NodeEditorView)
{
	// !TODO: Temporary
	mScene->setSceneRect(-200,-200,1000,600);

	// !TODO: Move to some initialize method
	mView->setScene(mScene);
	mView->setGeometry(100, 50, 1200, 800);
	connect(mView, SIGNAL(contextMenu(QPoint,QPointF)),
		this, SLOT(contextMenu(QPoint,QPointF)));
	mView->show();

	for(int i = 0; i < sizeof(registeredNodeTypes)/sizeof(QString); ++i)
	{
		QAction* a = new QAction(registeredNodeTypes[i], this);
		a->setData(i);
		mAddNodesActions.append(a);
	}
}

NodeController::~NodeController()
{
	foreach(NodeLinkView* link, mLinkViews)
		delete link;
	mLinkViews.clear();

	foreach(NodeView* node, mNodeViews.values())
		delete node;
	mNodeViews.clear();
}

NodeView* NodeController::nodeView(quint32 nodeKey)
{
	return mNodeViews[nodeKey];
}

NodeView* NodeController::addNodeView(quint32 nodeKey, const QString& title)
{
	NodeView* node = new NodeView(title);
	node->setData(NodeDataIndex::NodeKey, nodeKey);
	mNodeViews[nodeKey] = node;
	mScene->addItem(node);
	return node;
}

bool NodeController::deleteNodeView(quint32 nodeKey)
{
	qDebug() << "deleting node with key:" << nodeKey;
	return false;
}

void NodeController::linkNodeViews(NodeSocketView* from, NodeSocketView* to)
{
	// TODO!: To bedzie w logice
	// Check if we aren't trying to create already existing connection
	for(auto it = mLinkViews.begin(); it != mLinkViews.end(); ++it)
	{
		if((*it)->connects(from, to))
		{
			QMessageBox::critical
				(nullptr, "NodeView", "Connection already exists");
			return;
		}
	}

	NodeLinkView* link = new NodeLinkView(from, to, nullptr);
	link->setDrawDebug(DEBUG_LINKS);
	from->addLink(link);
	to->addLink(link);
	mLinkViews.push_back(link);
	mScene->addItem(link);
}

void NodeController::addNode(quint32 nodeTypeId, const QPointF& scenePos)
{
	if(nodeTypeId < sizeof(registeredNodeTypes)/sizeof(QString))
	{
		QString title = registeredNodeTypes[nodeTypeId];

		NodeView* node = addNodeView(nodeId, title);
		node->setPos(scenePos);

		if(title == "Image from disk")
		{
			node->addSocketView(0, "Output", true);
		}
		else if(title == "Gaussian blur")
		{
			node->addSocketView(0, "Input");
			node->addSocketView(1, "Sigma");
			node->addSocketView(0, "Output", true);
		}
		else if(title == "Preview window")
		{
			node->addSocketView(0, "Input");
		}
		else if(title == "Grayscale")
		{
			node->addSocketView(0, "Input");
			node->addSocketView(0, "Output", true);
		}
		else if(title == "Mixture of Gaussians")
		{
			node->addSocketView(0, "Frame");
			node->addSocketView(0, "Foreground mask", true);
		}

		nodeId += 1;
	}
}

void NodeController::initSampleScene()
{
	addNode(0, QPointF(-250,0));
	addNode(3, QPointF(-40,50));
	addNode(1, QPointF(150,100));
	addNode(4, QPointF(350,150));
	addNode(2, QPointF(600,100));
}

void NodeController::draggingLinkDropped(QGraphicsWidget* from, 
	QGraphicsWidget* to)
{
	linkNodeViews(static_cast<NodeSocketView*>(from), static_cast<NodeSocketView*>(to));
}

void NodeController::draggingLinkStarted(QGraphicsWidget* from)
{
	mScene->setDragging(true);
}

void NodeController::draggingLinkStopped(QGraphicsWidget* from)
{
	mScene->setDragging(false);
}

void NodeController::contextMenu(const QPoint& globalPos,
	const QPointF& scenePos)
{
	QList<QGraphicsItem*> items = mScene->items(scenePos, 
		Qt::ContainsItemShape, Qt::AscendingOrder);
	//If we clicked onto empty space
	if(items.isEmpty())
	{
		QMenu menu;
		QMenu* menuAddNode = menu.addMenu("Add node");
		foreach(QAction* a, mAddNodesActions)
			menuAddNode->addAction(a);
		QAction* ret = menu.exec(globalPos);
		if(ret != nullptr)
			addNode(ret->data().toInt(), scenePos);
	}
	else
	{
		for(int i = 0; i < items.size(); ++i)
		{
			if(items[i]->type() == NodeView::Type)
			{
				QMenu menu;
				QAction action("Delete node", nullptr);
				action.setData(10);
				menu.addAction(&action);
				QAction* ret = menu.exec(globalPos);
				if(ret != nullptr)
					;//deleteNodeView(items[i]->nodeKey());
				return;
			}
		}
	}
}

