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
#include <QGraphicsSimpleTextItem>

class NodeSocketView : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit NodeSocketView(const QString& title,
        bool isOutput, QGraphicsItem* parent = nullptr);
    virtual int type() const;

    void setConnectorBrushGradient(const QColor& start, const QColor& stop);
    void setConnectorAnnotation(const QString& annotation);
    void setConnectorToolTip(const QString& toolTip);

    // Przelicza i ustawia rozmiary i pozycje nazwy oraz "wtyczki"
    void updateLayout();

    // Zmienia aktywnosc gniazda
    void setActive(bool active);

    // Zwraca pozycje srodka widoku gniazda
    QPointF connectorCenterPos() const;

    bool isOutput() const;
    const QString& title() const;

    void addLink(NodeLinkView* link);
    void removeLink(NodeLinkView* link);

    // Zwraca klucz gniazda
    SocketID socketKey() const;

    // Zwraca widok wezla do ktorego nalezy gniazdo
    NodeView* nodeView() const;

    enum { Type = QGraphicsItem::UserType + 3 };

protected:
    virtual QVariant itemChange(GraphicsItemChange change,
        const QVariant& value);

private:
    QString mTitle;
    bool mIsOutput;

    QGraphicsSimpleTextItem* mLabel;
    NodeConnectorView* mConnector;
    QList<NodeLinkView*> mLinks;
};

inline int NodeSocketView::type() const
{ return NodeSocketView::Type; }

inline bool NodeSocketView::isOutput() const
{ return mIsOutput; }

inline const QString& NodeSocketView::title() const
{ return mTitle; }

inline SocketID NodeSocketView::socketKey() const
{ return data(NodeDataIndex::SocketKey).toInt(); }
