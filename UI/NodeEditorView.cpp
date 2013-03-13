#include "NodeEditorView.h"
#include <QWheelEvent>
#include <QScrollBar>

NodeEditorView::NodeEditorView(QWidget* parent)
	: QGraphicsView(parent)
	, mZoom(1.0f)
	, mPanning(false)
	, mOriginalCursor(viewport()->cursor())
	, mLastMouseEventPos(QPoint())
	, mPseudoInteractive(false)
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
	if(!mPanning && !isPseudoInteractive())
		emit contextMenu(event->globalPos(), mapToScene(event->pos()));
}

void NodeEditorView::keyPressEvent(QKeyEvent* event)
{
	if(!isPseudoInteractive())
		emit keyPress(event);
}

void NodeEditorView::setPseudoInteractive(bool allowed)
{
	mPseudoInteractive = !allowed;
}