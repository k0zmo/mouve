#pragma once

#include "Prerequisites.h"

#include <QGraphicsScene>

class NodeScene : public QGraphicsScene
{
public:
	NodeScene(QObject* parent = nullptr);
	void setDragging(bool dragging);

protected:
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

private:
	NodeConnectorView* mHovered;
	bool mDragging;
};

inline void NodeScene::setDragging(bool dragging)
{ mDragging = dragging; }