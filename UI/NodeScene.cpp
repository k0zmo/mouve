#include "NodeScene.h"
#include "NodeConnectorView.h"
#include "NodeStyle.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

NodeScene::NodeScene(QObject* parent)
	: QGraphicsScene(parent)
	, mHovered(nullptr)
	, mDragging(false)
{
	setBackgroundBrush(NodeStyle::SceneBackground);
}

void NodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if(mDragging)
	{
		QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
		if(item && item->type() == NodeConnectorView::Type)
		{
			mHovered = static_cast<NodeConnectorView*>(item);
			mHovered->setHighlight(true);
		}
		else if(mHovered)
		{
			mHovered->setHighlight(false);
			mHovered = nullptr;
		}
	}
	QGraphicsScene::mouseMoveEvent(event);
}

void NodeScene::removeItem(QGraphicsItem* item)
{
	mHovered = nullptr;
	QGraphicsScene::removeItem(item);
}

