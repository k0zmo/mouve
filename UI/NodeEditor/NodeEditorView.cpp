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

#include "NodeEditorView.h"

#include <QWheelEvent>
#include <QScrollBar>
#include <QMimeData>
#include <QStandardItem>

NodeEditorView::NodeEditorView(QWidget* parent)
    : QGraphicsView(parent)
    , mZoom(1.0f)
    , mPanning(false)
    , mOriginalCursor(viewport()->cursor())
    , mLastMouseEventPos(QPoint())
{
    setRenderHint(QPainter::Antialiasing);
    scale(mZoom, mZoom);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFocusPolicy(Qt::ClickFocus);
    setDragMode(QGraphicsView::RubberBandDrag);
    setAcceptDrops(true);
}

void NodeEditorView::setZoom(float zoom)
{
    if(scene() != nullptr)
    {
        mZoom = zoom;

        QTransform transformation;
        transformation.scale(mZoom, mZoom);
        setTransform(transformation);
    }
}

void NodeEditorView::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::MiddleButton)
    {
        mPanning = true;
        mLastMouseEventPos = event->pos();
        viewport()->setCursor(Qt::SizeAllCursor);
    }
    else if(!mPanning)
    {
        QGraphicsView::mousePressEvent(event);
    }
}

void NodeEditorView::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::MiddleButton)
    {
        mPanning = false;
        viewport()->setCursor(mOriginalCursor);
    }
    else
    {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void NodeEditorView::mouseMoveEvent(QMouseEvent* event)
{
    if(mPanning)
    {
        QPoint delta = event->pos() - mLastMouseEventPos;
        mLastMouseEventPos = event->pos();

        QScrollBar* hBar = horizontalScrollBar();
        QScrollBar* vBar = verticalScrollBar();

        hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
        vBar->setValue(vBar->value() - delta.y());
    }
    else
    {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void NodeEditorView::wheelEvent(QWheelEvent* event)
{
    if(event->orientation() == Qt::Vertical)
    {
        float zoom = mZoom + event->delta() * 0.001f;
        zoom = qMin(qMax(zoom, 0.3f), 3.0f);
        setZoom(zoom);
    }
}

void NodeEditorView::contextMenuEvent(QContextMenuEvent* event)
{
    if(!mPanning)
        emit contextMenu(event->globalPos(), mapToScene(event->pos()));
}

void NodeEditorView::keyPressEvent(QKeyEvent* event)
{
    emit keyPress(event);
}

void NodeEditorView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")))
        event->acceptProposedAction();
}

void NodeEditorView::dragMoveEvent(QDragMoveEvent* event)
{
    if(event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")))
        event->acceptProposedAction();
}

void NodeEditorView::dropEvent(QDropEvent* event)
{
    if(event->mimeData()->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")))
    {
        QByteArray encoded = event->mimeData()->data(
            QStringLiteral("application/x-qabstractitemmodeldatalist"));
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            int r, c;
            stream >> r >> c;
            if(r != 1 && c != 0)
                return;
            QScopedPointer<QStandardItem> item(new QStandardItem);
            stream >> *item;
            
            NodeTypeID typeId = item->data(Qt::UserRole).toUInt();
            if(typeId != InvalidNodeTypeID)
                emit nodeTypeDropped(typeId, mapToScene(event->pos()));
        }
    }
}
