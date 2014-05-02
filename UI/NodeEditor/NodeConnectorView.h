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

#pragma once

#include "../Prerequisites.h"

#include <QGraphicsWidget>
#include <QPropertyAnimation>
#include <QPen>

class NodeConnectorView : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(float penWidth READ penWidth WRITE setPenWidth)
public:
    explicit NodeConnectorView(bool isOutput, QGraphicsItem* parent = nullptr);
    
    virtual void paint(QPainter* painter, 
        const QStyleOptionGraphicsItem* option, QWidget *widget);
    virtual QRectF boundingRect() const;
    virtual int type() const;

    void setBrushGradient(const QColor& start, const QColor& stop);
    void setAnnotation(const QString& annotation);
    float penWidth() const;
    void setPenWidth(float penWidth);

    void setHighlight(bool highlight);
    QPointF centerPos() const;
    NodeSocketView* socketView() const;

    bool isOutput() const;

    enum { Type = QGraphicsItem::UserType + 4 };

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
    QRect mRect;
    QPen mPen;
    QBrush mBrush;
    QPropertyAnimation mAnimation;
    NodeLinkView* mTemporaryLink;
    NodeConnectorView* mHoveredConnector;
    QGraphicsSimpleTextItem* mAnnotation;
    bool mIsOutput;

private:
    NodeConnectorView* canDrop(const QPointF& scenePos);

signals:
    void draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*);
};

inline float NodeConnectorView::penWidth() const 
{ return mPen.widthF(); }

inline QRectF NodeConnectorView::boundingRect() const
{ return mRect; }

inline int NodeConnectorView::type() const
{ return NodeConnectorView::Type; }

inline bool NodeConnectorView::isOutput() const
{ return mIsOutput; }

inline QPointF NodeConnectorView::centerPos() const
{ return QPointF(mRect.width() * 0.5, mRect.height() * 0.5); }
