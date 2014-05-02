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
#include <QGraphicsDropShadowEffect>

class NodeView : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit NodeView(const QString& title, const QString& typeName,
        QGraphicsItem* parent = nullptr);
    virtual ~NodeView();

    virtual int type() const;
    virtual void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option, QWidget* widget);

    NodeID nodeKey() const;
    NodeSocketView* addSocketView(SocketID socketKey, 
        ENodeFlowDataType dataType,	const QString& title,
        bool isOutput = false);
    void updateLayout();
    void selectPreview(bool selected);

    enum { Type = QGraphicsItem::UserType + 1 };

    void setNodeViewName(const QString& newName);

    NodeSocketView* inputSocketView(SocketID socketID) const;
    NodeSocketView* outputSocketView(SocketID socketID) const;

    int outputSocketCount() const;
    SocketID previewSocketID() const;
    void setPreviewSocketID(SocketID socketID);

    void setTimeInfo(const QString& text);
    void setTimeInfoVisible(bool visible);

    void setNodeWithStateMark(bool visible);

    bool isNodeEnabled() const;
    void setNodeEnabled(bool enable);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);

private:
    QGraphicsTextItem* mLabel;
    QGraphicsTextItem* mTypeLabel;
    QGraphicsSimpleTextItem* mTimeInfo;
    QGraphicsItem* mStateMark;
    QGraphicsDropShadowEffect* mDropShadowEffect;
    QPainterPath mShape1;
    QPainterPath mShape2;
    QMap<SocketID, NodeSocketView*> mInputSocketViews;
    QMap<SocketID, NodeSocketView*> mOutputSocketViews;
    int mPreviewSocketID;
    bool mPreviewSelected;
    bool mStateMarkVisible;
    bool mNodeEnabled;

private:
    QPainterPath shape1(qreal titleHeight) const;
    QPainterPath shape2(qreal titleHeight) const;

signals:
    void mouseDoubleClicked(NodeView*);

    void mouseHoverEntered(NodeID nodeID, QGraphicsSceneHoverEvent* event);
    void mouseHoverLeft(NodeID nodeID, QGraphicsSceneHoverEvent* event);
};

inline int NodeView::type() const
{ return NodeView::Type; }

inline NodeID NodeView::nodeKey() const
{ return data(NodeDataIndex::NodeKey).toInt(); }

inline NodeSocketView* NodeView::inputSocketView(SocketID socketID) const
{ return mInputSocketViews.value(socketID); }

inline NodeSocketView* NodeView::outputSocketView(SocketID socketID) const
{ return mOutputSocketViews.value(socketID); }

inline int NodeView::outputSocketCount() const
{ return mOutputSocketViews.count(); }

inline SocketID NodeView::previewSocketID() const
{ return mPreviewSocketID; }

inline void NodeView::setPreviewSocketID(SocketID socketID)
{ mPreviewSocketID = socketID; }

inline bool NodeView::isNodeEnabled() const
{ return mNodeEnabled; }
