#pragma once

#include "Prerequisites.h"

#include <QGraphicsView>

class NodeEditorView : public QGraphicsView
{
	Q_OBJECT
public:
	NodeEditorView(QWidget* parent = nullptr);

	/// TODO: SLOTS?
	void setZoom(float zoom);
	void setPseudoInteractive(bool allowed);
	
	bool isPseudoInteractive() const;

protected:
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void contextMenuEvent(QContextMenuEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dragMoveEvent(QDragMoveEvent* event);
	virtual void dropEvent(QDropEvent* event);

private:
	float mZoom;
	bool mPanning;
	QCursor mOriginalCursor;
	QPoint mLastMouseEventPos;
	bool mPseudoInteractive;

signals:
	void contextMenu(const QPoint& globalPos, const QPointF& scenePos);
	void keyPress(QKeyEvent* event);
};

inline bool NodeEditorView::isPseudoInteractive() const
{ return mPseudoInteractive; }