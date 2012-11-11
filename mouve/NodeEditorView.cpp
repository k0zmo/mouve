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

	// !TODO: Make scroll wheel change it
	// setDragMode(QGraphicsView::ScrollHandDrag);
	setDragMode(QGraphicsView::RubberBandDrag);
}

void NodeEditorView::setZoom(float zoom)
{
	if(scene() != nullptr)
	{
		QTransform transformation;
		transformation.scale(mZoom, mZoom);
		setTransform(transformation);
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