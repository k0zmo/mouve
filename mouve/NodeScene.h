#pragma once

#include "Prerequisites.h"

#include <QGraphicsScene>

class NodeScene : public QGraphicsScene
{
public:
	NodeScene(QObject* parent = nullptr);
	bool setDragging(bool dragging);

protected:
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

private:
	bool mDragging;
	NodeConnectorView* mHovered;
};

inline bool NodeScene::setDragging(bool dragging)
{ mDragging = dragging; }