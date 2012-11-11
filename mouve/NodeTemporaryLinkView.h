#pragma once

#include "Prerequisites.h"

#include <QGraphicsItem>

class NodeTemporaryLinkView :  public QGraphicsItem
{
public:
	NodeTemporaryLinkView(const QPointF& startPosition,
		const QPointF& endPosition, QGraphicsItem* parent = nullptr);

	virtual void paint(QPainter *painter, 
		const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual int type() const;
	// !TODO: virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;

	void updateEndPosition(const QPointF& endPosition);

	enum
	{
		Type = QGraphicsItem::UserType + 5
	};

	void setDrawMode(int drawMode);
	void setDrawDebug(bool drawDebug);

private:
	bool mDrawDebug;
	//! TODO: Make it enum
	int mDrawMode;
	QPen mPen;
	QPointF mEndPosition;

private:
	QPainterPath _shape() const;
};

inline int NodeTemporaryLinkView::type() const
{ return NodeTemporaryLinkView::Type; }