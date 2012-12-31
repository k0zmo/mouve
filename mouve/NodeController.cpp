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

template<> NodeController* Singleton<NodeController>::_singleton = nullptr;

NodeController::NodeController(QObject* parent)
	: QObject(parent)
	, mScene(new NodeScene(this))
	, mView(new NodeEditorView)
{
	// !TODO: Temporary
	mScene->setSceneRect(-200,-200,1000,600);
	// Qt bug concering scene->removeItem ?? Seems to fixed it
	mScene->setItemIndexMethod(QGraphicsScene::NoIndex);

	// !TODO: Move to some initialize method
	mView->setScene(mScene);
	mView->setGeometry(100, 50, 1200, 800);
	connect(mView, SIGNAL(contextMenu(QPoint,QPointF)),
		this, SLOT(contextMenu(QPoint,QPointF)));
	connect(mView, SIGNAL(keyPress(QKeyEvent*)),
		this, SLOT(keyPress(QKeyEvent*)));
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

	delete mView;
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
	qDebug() << "deleting node view with nodeKey:" << nodeKey;

	auto it = mNodeViews.find(nodeKey);
	if(it != mNodeViews.end())
	{
		NodeView* node = *it;

		// Remove incoming and outgoing links
		QMutableListIterator<NodeLinkView*> itl(mLinkViews);
		while(itl.hasNext())
		{
			NodeLinkView* link = itl.next();

			if(link->inputConnecting(node) ||
			   link->outputConnecting(node))
			{
				mScene->removeItem(link);
				itl.remove();
				delete link;
			}
		}

		// Remove from the scene
		mScene->removeItem(node);
		// Remove from container
		mNodeViews.erase(it);
		// Remove at last node view itself
		node->deleteLater();
		return true;
	}
	return false;
}

bool NodeController::deleteNodeView(NodeView* nodeView)
{
	return deleteNodeView(
		quint32(nodeView->data(NodeDataIndex::NodeKey).toInt()));
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

bool NodeController::unlinkNodeViews(NodeSocketView* from, NodeSocketView* to)
{
	for(int i = 0; i < mLinkViews.size(); ++i)
	{
		if(mLinkViews[i]->connects(from, to))
		{
			NodeLinkView* link = mLinkViews[i];
			mScene->removeItem(link);
			mLinkViews.removeAt(i);
			delete link;			
			return true;
		}
	}

	return false;
}

bool NodeController::unlinkNodeViews(NodeLinkView* link)
{
	if(mLinkViews.removeOne(link))
	{
		mScene->removeItem(link);
		delete link;
		return true;
	}
	return false;
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
				{
					NodeView* node = static_cast<NodeView*>(items[i]);
					deleteNodeView(node->nodeKey());
				}
				return;
			}
		}
	}
}

void NodeController::keyPress(QKeyEvent* event)
{
	if(event->key() == Qt::Key_Delete)
	{
		QList<QGraphicsItem*> itemsPing = mScene->selectedItems();
		QList<QGraphicsItem*> itemsPong;
		mScene->clearSelection();

		for(int i = 0; i < itemsPing.size(); ++i)
		{
			if(itemsPing[i]->type() == NodeLinkView::Type)
			{
				NodeLinkView* link = static_cast<NodeLinkView*>(itemsPing[i]);
				unlinkNodeViews(link);
			}
			else
			{
				itemsPong.append(itemsPing[i]);
			}
		}
		itemsPing.clear();

		for(int i = 0; i < itemsPong.size(); ++i)
		{
			if(itemsPong[i]->type() == NodeView::Type)
			{
				NodeView* node = static_cast<NodeView*>(itemsPong[i]);
				deleteNodeView(node);
			}
		}

		event->accept();
	}
}

