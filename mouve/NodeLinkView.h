#pragma once

#include "Prerequisites.h"

#include <QGraphicsItem>

class NodeLinkView :  public QGraphicsItem
{
public:
	NodeLinkView(NodeSocketView* fromSocketView, 
		NodeSocketView* toSocketView, QGraphicsItem* parent = nullptr);
	virtual ~NodeLinkView();

	virtual void paint(QPainter *painter, 
		const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual int type() const;
	// !TODO: virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;

	// Uaktualnia ksztlat i pozycje na podstawie gniazd do ktorych wchodzi/wychodzi
	void updateFromSocketViews();

	// Zwraca true jesli obiekt laczy podane gniazda
	bool connects(NodeSocketView* from, NodeSocketView* to) const;
	// Zwraca true jesli obiekt wychodzi z podanego wezla 
	bool outputConnecting(NodeView* from) const;
	// Zwraca true jesli obiektu wchodzi do podanego wezla
	bool inputConnecting(NodeView* to) const;

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
	NodeSocketView* mFromSocketView;
	NodeSocketView* mToSocketView;

private:
	// Buduje i zwraca QPainterPath 
	QPainterPath _shape() const;
};

inline int NodeLinkView::type() const
{ return NodeLinkView::Type; }