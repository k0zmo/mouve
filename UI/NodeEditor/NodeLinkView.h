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

#include <QGraphicsItem>
#include <QPen>

class NodeLinkView :  public QGraphicsItem
{
public:
    explicit NodeLinkView(NodeSocketView* fromSocketView,
        NodeSocketView* toSocketView, QGraphicsItem* parent = nullptr);
    // Contruction for temporary link 
    explicit NodeLinkView(const QPointF& startPosition,
        const QPointF& endPosition, QGraphicsItem* parent = nullptr,
        bool invertStartPos = false);
    virtual ~NodeLinkView();

    virtual void paint(QPainter *painter, 
        const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual int type() const;
    virtual QPainterPath shape() const;
    virtual QRectF boundingRect() const;

    void setBad(bool bad);

    // Uaktualnia ksztlat i pozycje na podstawie gniazd do ktorych wchodzi/wychodzi
    void updateFromSocketViews();
    void updateEndPosition(const QPointF& endPosition);

    // Zwraca true jesli obiekt laczy podane gniazda
    bool connects(NodeSocketView* from, NodeSocketView* to) const;
    // Zwraca true jesli obiekt wychodzi z podanego wezla 
    bool outputConnecting(NodeView* from) const;
    // Zwraca true jesli obiektu wchodzi do podanego wezla
    bool inputConnecting(NodeView* to) const;

    enum { Type = QGraphicsItem::UserType + 2 };

    void setDrawDebug(bool drawDebug);

    const NodeSocketView* fromSocketView() const;
    const NodeSocketView* toSocketView() const;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:
    QPen mPen;
    QPointF mEndPosition;
    NodeSocketView* mFromSocketView;
    NodeSocketView* mToSocketView;
    QPainterPath mPath;
    mutable QPointF mc1, mc2;
    bool mDrawDebug;
    bool mInvertStartPos;
    bool mBadConnection;

private:
    // Buduje i zwraca QPainterPath 
    QPainterPath shapeImpl() const;
};

inline int NodeLinkView::type() const
{ return NodeLinkView::Type; }

inline const NodeSocketView* NodeLinkView::fromSocketView() const
{ return mFromSocketView; }

inline const NodeSocketView* NodeLinkView::toSocketView() const
{ return mToSocketView; }
