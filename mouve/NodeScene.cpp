#include "NodeScene.h"
#include "NodeConnectorView.h"
#include "NodeStyle.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

#include <QDebug>

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
		QGraphicsItem* item = itemAt(event->scenePos());
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