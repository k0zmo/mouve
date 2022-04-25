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

#include "NodeToolTip.h"
#include "NodeStyle.h"

#include <QPainter>

NodeToolTip::NodeToolTip(QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , _text()
    , _boundingRect(QRectF())
{
    setCacheMode(DeviceCoordinateCache);
    setZValue(300);
}

QRectF NodeToolTip::boundingRect() const
{
    return _boundingRect;
}

void NodeToolTip::paint(QPainter* painter, 
           const QStyleOptionGraphicsItem* option,
           QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF textRect = boundingRect().adjusted(10, 10, -10, -10);
    int flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap;

    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(QColor(245, 245, 255, 180));
    painter->setClipRect(boundingRect());
    painter->drawRect(boundingRect());

    painter->setPen(Qt::black);
    painter->setFont(NodeStyle::SocketFont);
    painter->drawText(textRect, flags, _text);

}

void NodeToolTip::setText(const QString& text)
{
    _text = text;

    // Calculate bounding rect
    QFontMetrics metrics(NodeStyle::SocketFont);
    QRect textRect = QRect(0, 0, 250, 1500);
    int flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap;
    _boundingRect = metrics.boundingRect(textRect, flags, text)
        .adjusted(-50, -50, 10, 10);

    update();
}
