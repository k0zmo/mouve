#include "NodeLinkView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"

#include <QPainter>

NodeLinkView::NodeLinkView(NodeSocketView* fromSocketView, 
	NodeSocketView* toSocketView, QGraphicsItem* parent)
	: QGraphicsItem(parent)
	, mPen(NodeStyle::LinkPen)
	, mFromSocketView(fromSocketView)
	, mToSocketView(toSocketView)
	, mDrawDebug(false)
{
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemIsFocusable);
	setZValue(NodeStyle::ZValueLink);
	updateFromSocketViews();
}

NodeLinkView::~NodeLinkView()
{
	// Remove the references so the socket views won't point to the no man's land
	if(mFromSocketView)
		mFromSocketView->removeLink(this);
	if(mToSocketView)
		mToSocketView->removeLink(this);
}

void NodeLinkView::paint(QPainter *painter,
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

QRectF NodeLinkView::boundingRect() const
{
	return mPath.controlPointRect();
}

void NodeLinkView::setDrawDebug(bool drawDebug)
{
	mDrawDebug = drawDebug;
	mPath = shapeImpl();
	update();
}

QVariant NodeLinkView::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if(change == QGraphicsItem::ItemSelectedHasChanged)
	{
		if(value.toBool())
		{
			mPen.setStyle(Qt::DotLine);
			update();
		}
		else
		{
			mPen.setStyle(Qt::SolidLine);
			update();
		}
	}
	return value;
}

void NodeLinkView::updateFromSocketViews()
{
	if(mFromSocketView && mToSocketView)
	{
		// NOTE: Origin is always at 0,0 in item coordinates
		prepareGeometryChange();
		setPos(mFromSocketView->scenePos() + mFromSocketView->connectorCenterPos());
		mEndPosition = mapFromScene(mToSocketView->scenePos() + mToSocketView->connectorCenterPos());
		mPath = shapeImpl();
	}
}

bool NodeLinkView::connects(NodeSocketView* from, NodeSocketView* to) const
{
	return from == mFromSocketView && to == mToSocketView;
}

bool NodeLinkView::outputConnecting(NodeView* from) const
{
	if(mFromSocketView)
		return from == mFromSocketView->nodeView();
	return false;
}

bool NodeLinkView::inputConnecting(NodeView* to) const
{
	if(mToSocketView)
		return to == mToSocketView->nodeView();
	return false;
}

QPainterPath NodeLinkView::shape() const
{
	return shapeImpl();
}

QPainterPath NodeLinkView::shapeImpl() const
{
	int drawMode = 1;
	// 0,0 (origin) is at startPosition
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
