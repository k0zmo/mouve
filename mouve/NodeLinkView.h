#pragma once

#include "Prerequisites.h"

#include <QGraphicsItem>

class NodeLinkView :  public QGraphicsItem
{
public:
	NodeLinkView(const NodeSocketView* fromSocketView, 
		const NodeSocketView* toSocketView, QGraphicsItem* parent = nullptr);
	virtual ~NodeLinkView();

	virtual void paint(QPainter *painter, 
		const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual int type() const;
	// !TODO: virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;

	// Uaktualnia ksztlat i pozycje na podstawie gniazd do ktorych wchodzi/wychodzi
	void updateFromSocketViews();

	enum
	{
		Type = QGraphicsItem::UserType + 2
	};

	// Zmiana trybu rysowania krzywej
	void setDrawMode(int drawMode);
	void setDrawDebug(bool drawDebug);

protected:
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:
	bool mDrawDebug;
	//! TODO: Make it enum
	int mDrawMode;
	QPen mPen;
	QPointF mEndPosition;
	const NodeSocketView* mFromSocketView;
	const NodeSocketView* mToSocketView;

private:
	// Buduje i zwraca QPainterPath 
	QPainterPath _shape() const;
};

inline int NodeLinkView::type() const
{ return NodeLinkView::Type; }