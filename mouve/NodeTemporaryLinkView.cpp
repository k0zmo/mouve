#include "NodeTemporaryLinkView.h"
#include "NodeStyle.h"

NodeTemporaryLinkView::NodeTemporaryLinkView(const QPointF& startPosition,
	const QPointF& endPosition, QGraphicsItem* parent)
	: QGraphicsItem(parent)
	, mDrawDebug(false)
	, mDrawMode(1)
	, mPen(NodeStyle::LinkPen)
	, mEndPosition(mapFromScene(endPosition))
{
	setPos(startPosition);
	setZValue(NodeStyle::ZValueTemporaryLink);
}

void NodeTemporaryLinkView::paint(QPainter *painter,
	const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setBrush(Qt::NoBrush);
	painter->setPen(mPen);
	painter->drawPath(_shape());
	// Draw debugging red-ish rectangle
	if(mDrawDebug)
	{
		painter->setBrush(QColor(255,0,0,50));
		painter->setPen(Qt::red);
		painter->drawRect(boundingRect());
	}
}

QRectF NodeTemporaryLinkView::boundingRect() const
{
	// !TODO: zbadac to
	float d = 0.0f;
	float x1 = qMin(qreal(0), mEndPosition.x()) - d;
	float y1 = qMin(qreal(0), mEndPosition.y()) - d;
	float x2 = qMax(qreal(0), mEndPosition.x()) + d;
	float y2 = qMax(qreal(0), mEndPosition.y()) + d;
	return QRectF(QPointF(x1, y1), QPointF(x2, y2));
}

void NodeTemporaryLinkView::updateEndPosition(const QPointF& endPosition)
{
	if(mEndPosition != endPosition)
	{
		prepareGeometryChange();
		mEndPosition = mapFromScene(endPosition);
	}
}

void NodeTemporaryLinkView::setDrawMode(int drawMode)
{
	mDrawMode = drawMode;
	update();
}

void NodeTemporaryLinkView::setDrawDebug(bool drawDebug)
{
	mDrawDebug = drawDebug;
	update();
}

QPainterPath NodeTemporaryLinkView::_shape() const
{
	// Default bezier drawing NodeStyle
	if(mDrawMode == 1)
	{
		QPointF c1(mEndPosition.x() / 2.0, 0);
		QPointF c2(mEndPosition.x() / 2.0, mEndPosition.y());

		QPainterPath painterPath;
		painterPath.cubicTo(c1, c2, mEndPosition);
		return painterPath;
	}
	//
	// Not quite working ;p 
	//
	else if(mDrawMode == 2)
	{
		qreal tangentLength = qAbs(mEndPosition.x()) / 2.0 +
			qAbs(mEndPosition.y()) / 4.0;
		QPointF startTangent(tangentLength, 0.0);
		QPointF endTangent(mEndPosition.x() - tangentLength, mEndPosition.y());

		QPainterPath painterPath;
		painterPath.cubicTo(startTangent, endTangent, mEndPosition);
		return painterPath;
	}
	// Simple line
	else
	{
		QPainterPath painterPath(QPointF(0, 0));
		painterPath.lineTo(mEndPosition);
		return painterPath;
	}	
}