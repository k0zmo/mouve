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

#include "NodeSocketView.h"
#include "NodeStyle.h"
#include "NodeConnectorView.h"
#include "NodeView.h"
#include "NodeLinkView.h"

NodeSocketView::NodeSocketView(const QString& title, bool isOutput,
    QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , mTitle(title)
    , mIsOutput(isOutput)
    , mLabel(new QGraphicsSimpleTextItem(this))
    , mConnector(new NodeConnectorView(isOutput, this))
{
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

    mLabel->setBrush(NodeStyle::SocketTitleBrush);
    mLabel->setText(title);
    mLabel->setFont(NodeStyle::SocketFont);
}

void NodeSocketView::setConnectorBrushGradient(const QColor& start, const QColor& stop)
{
    mConnector->setBrushGradient(start, stop);
}

void NodeSocketView::setConnectorAnnotation(const QString& annotation)
{
    mConnector->setAnnotation(annotation);
}

void NodeSocketView::setConnectorToolTip(const QString& toolTip)
{
    mConnector->setToolTip(toolTip);
}

void NodeSocketView::updateLayout()
{
    const QRectF& b = mLabel->boundingRect();
    const QRectF& bb = mConnector->boundingRect();
    qreal h = (b.height() - bb.height()) / 2;
    qreal height = qMax(bb.height(), b.height());
    qreal width = b.width() + qreal(3.0) + bb.width();

    if(isOutput())
    {
        mConnector->setPos(b.width() + 3.0, h);
        mLabel->setPos(0, mLabel->pos().y());
    }
    else
    {
        mConnector->setPos(0, h);
        mLabel->setPos(bb.width() + 3.0, mLabel->pos().y());
    }
    resize(width, height);
}

void NodeSocketView::setActive(bool active)
{
    mLabel->setBrush(active 
        ? NodeStyle::SocketTitleBrush 
        : NodeStyle::SocketTitleInactiveBrush);
}

QPointF NodeSocketView::connectorCenterPos() const
{ 
    return mConnector->pos() + mConnector->centerPos();
}

QVariant NodeSocketView::itemChange(GraphicsItemChange change,
    const QVariant& value)
{
    // Update link views connecting this socket view
    if (change == QGraphicsItem::ItemScenePositionHasChanged)
    {
        foreach(NodeLinkView* link, mLinks)
            link->updateFromSocketViews();
    }
    // Default implementation does nothing and return 'value'
    return value;
}

void NodeSocketView::addLink(NodeLinkView* link)
{
    mLinks.append(link);
}

void NodeSocketView::removeLink(NodeLinkView* link)
{
    mLinks.removeOne(link);
}

NodeView* NodeSocketView::nodeView() const
{ 
    return static_cast<NodeView*>(parentObject());
}