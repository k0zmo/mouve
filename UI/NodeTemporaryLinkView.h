#pragma once

#include "Prerequisites.h"

#include <QGraphicsItem>

class NodeTemporaryLinkView :  public QGraphicsItem
{
public:
	explicit NodeTemporaryLinkView(const QPointF& startPosition,
		const QPointF& endPosition, QGraphicsItem* parent = nullptr);

	virtual void paint(QPainter *painter, 
		const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual int type() const;
	/// xXx: virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;

	void updateEndPosition(const QPointF& endPosition);

	enum { Type = QGraphicsItem::UserType + 5 };

	void setDrawMode(int drawMode);
	void setDrawDebug(bool drawDebug);

private:
	QPen mPen;
	QPointF mEndPosition;

	bool mDrawDebug;
	/// xXx: Make it enum
	int mDrawMode;

private:
	QPainterPath shapeImpl() const;
};

inline int NodeTemporaryLinkView::type() const
{ return NodeTemporaryLinkView::Type; }
