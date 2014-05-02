/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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
    , mInvertStartPos(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setZValue(NodeStyle::ZValueLink);
    updateFromSocketViews();
}

NodeLinkView::NodeLinkView(const QPointF& startPosition,
    const QPointF& endPosition, QGraphicsItem* parent,
    bool invertStartPos)
    : QGraphicsItem(parent)
    , mPen(NodeStyle::TemporaryLinkPen)
    , mEndPosition(mapFromScene(endPosition))
    , mFromSocketView(nullptr)
    , mToSocketView(nullptr)
    , mDrawDebug(false)
    , mInvertStartPos(invertStartPos)
{
    setPos(startPosition);
    setZValue(NodeStyle::ZValueTemporaryLink);
    mPath = shapeImpl();
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

        // Draw control points
        painter->setPen(QPen(Qt::yellow, 5));
        QPointF p[] = { mc1, mc2 };
        painter->drawPoints(p, 2);
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

void NodeLinkView::updateEndPosition(const QPointF& endPosition)
{
    if(mEndPosition != endPosition)
    {
        prepareGeometryChange();
        mEndPosition = mapFromScene(endPosition);
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
    // Calculations are done in world space ("scene" space)
    QPointF start = mapToScene(QPointF(0,0));
    QPointF end = mapToScene(mEndPosition);

    if(mInvertStartPos)
        qSwap(start, end);

    if(end.x() > start.x())
    {
        qreal midpoint = (start.x() + end.x()) / 2.0;
        QPointF c1(midpoint, start.y());
        QPointF c2(midpoint, end.y());

        mc1 = mapFromScene(c1);
        mc2 = mapFromScene(c2);

        QPainterPath painterPath;
        painterPath.moveTo(mapFromScene(start));
        painterPath.cubicTo(mc1, mc2, mapFromScene(end));
        return painterPath;
    }
    else
    {
        qreal w = qAbs(end.x() - start.x()) * 1.25 + 20.0;
        qreal z = qAbs(end.y() - start.y()) * 0.2;
        z *= start.y() > end.y() ? -1 : 1;
        QPointF c1(start.x() + w, start.y() + z);
        QPointF c2(end.x() - w,   end.y() - z);

        mc1 = mapFromScene(c1);
        mc2 = mapFromScene(c2);

        QPainterPath painterPath;
        painterPath.moveTo(mapFromScene(start));
        painterPath.cubicTo(mc1, mc2, mapFromScene(end));
        return painterPath;
    }
}
