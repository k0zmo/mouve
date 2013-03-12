#include "NodeTemporaryLinkView.h"
#include "NodeStyle.h"

#include <QPainter>

NodeTemporaryLinkView::NodeTemporaryLinkView(const QPointF& startPosition,
	const QPointF& endPosition, QGraphicsItem* parent)
	: QGraphicsItem(parent)
	, mPen(NodeStyle::TemporaryLinkPen)
	, mEndPosition(mapFromScene(endPosition))
	, mDrawDebug(false)
{
	setPos(startPosition);
	setZValue(NodeStyle::ZValueTemporaryLink);

	mPath = shapeImpl();
}

void NodeTemporaryLinkView::paint(QPainter *painter,
	const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setBrush(Qt::NoBrush);
	painter->setPen(mPen);
	painter->drawPath(mPath);

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
	return mPath.controlPointRect();
}

void NodeTemporaryLinkView::updateEndPosition(const QPointF& endPosition)
{
	if(mEndPosition != endPosition)
	{
		prepareGeometryChange();
		mEndPosition = mapFromScene(endPosition);
		mPath = shapeImpl();
	}
}

void NodeTemporaryLinkView::setDrawDebug(bool drawDebug)
{
	mDrawDebug = drawDebug;
	mPath = shapeImpl();
	update();
}

QPainterPath NodeTemporaryLinkView::shapeImpl() const
{
	int drawMode = 1;
	if((qAbs(mEndPosition.y()) / qAbs(mEndPosition.x()) > 2.5)
		|| mEndPosition.x() < 0.0)
	{
		drawMode = 2;
	}

	if(drawMode == 1)
	{
		QPointF c1(mEndPosition.x() / 2.0, 0);
		QPointF c2(mEndPosition.x() / 2.0, mEndPosition.y());

		QPainterPath painterPath;
		painterPath.cubicTo(c1, c2, mEndPosition);
		return painterPath;
	}
	else
	{
		qreal tangentLength = qAbs(mEndPosition.x()) / 2.0 +
			qAbs(mEndPosition.y()) / 4.0;
		QPointF startTangent(tangentLength, 0.0);
		QPointF endTangent(mEndPosition.x() - tangentLength, mEndPosition.y());

		QPainterPath painterPath;
		painterPath.cubicTo(startTangent, endTangent, mEndPosition);
		return painterPath;
	}
}
