#pragma once

#include "Prerequisites.h"

#include <QGraphicsView>

class NodeEditorView : public QGraphicsView
{
	Q_OBJECT
public:
	NodeEditorView(QWidget* parent = nullptr);
	void setZoom(float zoom);

protected:
	virtual void wheelEvent(QWheelEvent* event);
	virtual void contextMenuEvent(QContextMenuEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);

private:
	float mZoom;

signals:
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);
};