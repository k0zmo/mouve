#include "NodeEditorView.h"
#include <QWheelEvent>

NodeEditorView::NodeEditorView(QWidget* parent)
	: QGraphicsView(parent)
	, mZoom(1.0f)
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
	/// xXx: For now this works not entirely like desired :)
	//if(event->button() == Qt::MiddleButton)
	//{
	//	setDragMode(QGraphicsView::ScrollHandDrag);
	//}
	//else
	//{
		QGraphicsView::mousePressEvent(event);
	//}
}

void NodeEditorView::mouseReleaseEvent(QMouseEvent* event)
{
	if(event->button() == Qt::MiddleButton)
	{
		setDragMode(QGraphicsView::RubberBandDrag);
	}
	else
	{
		QGraphicsView::mouseReleaseEvent(event);
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
	emit contextMenu(event->globalPos(), mapToScene(event->pos()));
}

void NodeEditorView::keyPressEvent(QKeyEvent* event)
{
	emit keyPress(event);
}
