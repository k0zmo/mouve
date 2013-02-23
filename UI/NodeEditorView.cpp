#include "NodeEditorView.h"
#include <QWheelEvent>
#include <QScrollBar>

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
}

void NodeEditorView::setZoom(float zoom)
{
	if(scene() != nullptr)
	{
		QTransform transformation;
		transformation.scale(zoom, zoom);
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
		QScrollBar* hBar = horizontalScrollBar();
		QScrollBar* vBar = verticalScrollBar();

		QPoint delta = event->pos() - mLastMouseEventPos;
		mLastMouseEventPos = event->pos();

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
		mZoom += event->delta() * 0.001f;
		mZoom = qMin(qMax(mZoom, 0.3f), 3.0f);
		setZoom(mZoom);
	}
}

void NodeEditorView::contextMenuEvent(QContextMenuEvent* event)
{
	if(!mPanning)
	{
		emit contextMenu(event->globalPos(), mapToScene(event->pos()));
	}	
}

void NodeEditorView::keyPressEvent(QKeyEvent* event)
{
	emit keyPress(event);
}
