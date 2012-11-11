#include "NodeConnectorView.h"
#include "NodeStyle.h"
#include "NodeTemporaryLinkView.h"

#include <QGraphicsSceneMouseEvent>

NodeConnectorView::NodeConnectorView(bool isOutput, QGraphicsItem* parent)
	: QGraphicsWidget(parent, 0)
	, mIsOutput(isOutput)
	, mRect(NodeStyle::SocketSize)
	, mPen(NodeStyle::SocketPen)
	, mAnimation(this, "penWidth")
	, mTemporaryLink(nullptr)
{
	QLinearGradient gradient(0, mRect.y(), 0, mRect.height());
	gradient.setColorAt(0, NodeStyle::SocketGradientStart);
	gradient.setColorAt(1, NodeStyle::SocketGradientStop);
	mBrush = QBrush(gradient);
	mPen.setWidthF(1.0f); // !TODO: Use style

	setAcceptHoverEvents(true);

	mAnimation.setDuration(250);
	mAnimation.setEasingCurve(QEasingCurve::InOutQuad);
	mAnimation.setStartValue(penWidth());
}

void NodeConnectorView::setPenWidth(float penWidth)
{
	mPen.setWidthF(penWidth);
	update();
}

void NodeConnectorView::paint(QPainter* painter,
	const QStyleOptionGraphicsItem* option, QWidget* widget )
{
	painter->setBrush(mBrush);
	painter->setPen(mPen);
	painter->drawEllipse(mRect);
}

void NodeConnectorView::setHighlight(bool highlight)
{
	if(highlight)
	{
		mAnimation.setStartValue(mAnimation.currentValue());
		mAnimation.setEndValue(2.0); // !TODO: Use style
		mAnimation.start();
	}
	else
	{
		mAnimation.setStartValue(mAnimation.currentValue());
		mAnimation.setEndValue(1.0); // !TODO: Use style
		mAnimation.start();
	}
}

void NodeConnectorView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	setHighlight(true);
}

void NodeConnectorView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	setHighlight(false);
}

void NodeConnectorView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if(event->button() == Qt::LeftButton)
	{
		if(isOutput()) // !TODO: or !isOutput()
		{ 
			mTemporaryLink = new NodeTemporaryLinkView
				(centerPos(), event->scenePos(), this);
			emit draggingLinkStarted(socketView());
		}
	}
}

void NodeConnectorView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if(mTemporaryLink != nullptr)
		mTemporaryLink->updateEndPosition(event->scenePos());
}

void NodeConnectorView::mouseReleaseEvent( QGraphicsSceneMouseEvent* event)
{
	if(mTemporaryLink != nullptr)
	{
		emit draggingLinkStopped(socketView());
		NodeConnectorView* itemColliding = canDrop(event->scenePos());
		if(itemColliding != nullptr)
			emit draggingLinkDropped(socketView(), itemColliding->socketView());
		scene()->removeItem(mTemporaryLink);
		delete mTemporaryLink;
		mTemporaryLink = nullptr;
	}
}

NodeConnectorView* NodeConnectorView::canDrop(const QPointF& scenePos)
{
	QList<QGraphicsItem*> items = 
		scene()->items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder);
	foreach(QGraphicsItem* item, items)
	{
		// Did we dropped on another connector
		if(item->type() == NodeConnectorView::Type && item != this)
		{
			// Ensure we connect from input to output or the opposite
			NodeConnectorView* citem = static_cast<NodeConnectorView*>(item);
			if(citem->isOutput() != isOutput())
			{
				// Protect from constructing algebraic loops
				// !TODO: move to logic
				if(citem->socketView()->nodeView() != socketView()->nodeView())
				{
					return citem;
				}
			}
		}
	}
	return nullptr;
}


