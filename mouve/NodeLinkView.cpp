#include "NodeLinkView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"

NodeLinkView::NodeLinkView(NodeSocketView* fromSocketView, 
	NodeSocketView* toSocketView, QGraphicsItem* parent)
	: QGraphicsItem(parent)
	, mDrawDebug(false)
	, mDrawMode(1)
	, mPen(NodeStyle::LinkPen)
	, mFromSocketView(fromSocketView)
	, mToSocketView(toSocketView)
{
	// !TODO: Dla usuwania wezlow potrzebna jest mozliwosc ich zaznaczenia
	//        Na razie jednak kolizja dziala w oparciu o boundingRect 
	//        co nie bardzo nas zadowola dla cienkich linii
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemIsFocusable);
	setZValue(NodeStyle::ZValueLink);
	updateFromSocketViews();
}

NodeLinkView::~NodeLinkView()
{
	// Remove the references so we socket views won't point to the no man's land
	if(mFromSocketView)
		mFromSocketView->removeLink(this);
	if(mToSocketView)
		mToSocketView->removeLink(this);
}

void NodeLinkView::paint(QPainter *painter,
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

QRectF NodeLinkView::boundingRect() const
{
	float x1 = qMin(qreal(0), mEndPosition.x());
	float y1 = qMin(qreal(0), mEndPosition.y());
	float x2 = qMax(qreal(0), mEndPosition.x());
	float y2 = qMax(qreal(0), mEndPosition.y());
	return QRectF(QPointF(x1, y1), QPointF(x2, y2));
}

void NodeLinkView::setDrawMode(int drawMode)
{
	mDrawMode = drawMode;
	update();
}

void NodeLinkView::setDrawDebug(bool drawDebug)
{
	mDrawDebug = drawDebug;
	update();
}

QPainterPath NodeLinkView::_shape() const
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