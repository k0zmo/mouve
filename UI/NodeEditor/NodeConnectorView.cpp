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

#include "NodeConnectorView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"
#include "NodeLinkView.h"

#include "UI/Controller.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPainter>

NodeConnectorView::NodeConnectorView(bool isOutput, QGraphicsItem* parent)
    : QGraphicsWidget(parent, 0)
    , mRect(NodeStyle::SocketSize)
    , mPen(NodeStyle::SocketPen)
    , mAnimation(this, "penWidth")
    , mTemporaryLink(nullptr)
    , mHoveredConnector(nullptr)
    , mAnnotation(new QGraphicsSimpleTextItem(this))
    , mIsOutput(isOutput)
{
    setBrushGradient(Qt::white, Qt::black);
    setPenWidth(NodeStyle::NodeSocketPenWidth);
    setAnnotation(QString());
    setCursor(Qt::PointingHandCursor);

    setAcceptHoverEvents(true);

    mAnimation.setDuration(250);
    mAnimation.setEasingCurve(QEasingCurve::InOutQuad);
    mAnimation.setStartValue(penWidth());

    connect(this, SIGNAL(draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*)),
        Controller::instancePtr(), SLOT(draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*)));
}

void NodeConnectorView::setBrushGradient(const QColor& start, const QColor& stop)
{
    QLinearGradient gradient(0, mRect.y(), 0, mRect.height());
    gradient.setColorAt(0, start);
    gradient.setColorAt(1, stop);
    mBrush = QBrush(gradient);
    update();
}

void NodeConnectorView::setAnnotation(const QString& annotation)
{
    if(annotation.isEmpty())
    {
        mAnnotation->setVisible(false);
    }
    else
    {
        mAnnotation->setVisible(true);
        mAnnotation->setFont(NodeStyle::SocketAnnotationFont);
        mAnnotation->setBrush(NodeStyle::SocketAnnotationBrush);
        mAnnotation->setText(annotation);
        QRectF abbect = mAnnotation->boundingRect();
        QRectF brect = boundingRect();	
        mAnnotation->setPos((brect.width() - abbect.width()) * 0.5,
            (brect.height() - abbect.height()) * 0.5);
    }
    update();
}

void NodeConnectorView::setPenWidth(float penWidth)
{
    mPen.setWidthF(penWidth);
    update();
}

void NodeConnectorView::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(mBrush);
    painter->setPen(mPen);
    painter->drawEllipse(mRect);
}

void NodeConnectorView::setHighlight(bool highlight)
{
    if(highlight)
    {
        mAnimation.stop();
        qreal start = qMax(qreal(1.0), mAnimation.currentValue().toReal());
        mAnimation.setStartValue(start);
        mAnimation.setEndValue(NodeStyle::NodeSocketPenWidthHovered);
        mAnimation.start();
    }
    else
    {
        mAnimation.stop();
        qreal start = qMin(qreal(2.0), mAnimation.currentValue().toReal());
        mAnimation.setStartValue(start);
        mAnimation.setEndValue(NodeStyle::NodeSocketPenWidth);
        mAnimation.start();
    }
}

void NodeConnectorView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    setHighlight(true);
}

void NodeConnectorView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    setHighlight(false);
}

void NodeConnectorView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton
        && !mTemporaryLink) // protects from double click
    {
        mTemporaryLink = new NodeLinkView(centerPos(), event->scenePos(), this, !mIsOutput);
    }
}

void NodeConnectorView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(mTemporaryLink != nullptr)
    {
        mTemporaryLink->updateEndPosition(event->scenePos());

        QList<QGraphicsItem*> items = scene()->items
            (event->scenePos(), Qt::IntersectsItemShape, Qt::DescendingOrder);
        for(QGraphicsItem* item : items)
        {
            // Did we hover on another connector during making connection
            if(item != this && item->type() == NodeConnectorView::Type)
            {
                auto nc = static_cast<NodeConnectorView*>(item);
                if(nc->isOutput() != isOutput())
                {
                    if(mHoveredConnector != nullptr)
                        mHoveredConnector->setHighlight(false);

                    mHoveredConnector = nc;
                    mHoveredConnector->setHighlight(true);
                    return;
                }
            }
        }

        if(mHoveredConnector != nullptr)
        {
            mHoveredConnector->setHighlight(false);
            mHoveredConnector = nullptr;
        }
    }
}

void NodeConnectorView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(mTemporaryLink != nullptr)
    {
        NodeConnectorView* itemColliding = canDrop(event->scenePos());
        if(itemColliding != nullptr)
        {
            if(isOutput())
                emit draggingLinkDropped(socketView(), itemColliding->socketView());
            else
                emit draggingLinkDropped(itemColliding->socketView(), socketView());
        }

        mHoveredConnector = nullptr;

        // From Qt: It is more efficient to remove the item from the QGraphicsScene before destroying the item.
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
                return citem;
            }
        }
    }
    return nullptr;
}

NodeSocketView* NodeConnectorView::socketView() const
{
    return static_cast<NodeSocketView*>(parentObject());
}
