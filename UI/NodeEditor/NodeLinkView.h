#pragma once

#include "../Prerequisites.h"

#include <QGraphicsItem>
#include <QPen>

class NodeLinkView :  public QGraphicsItem
{
public:
	explicit NodeLinkView(NodeSocketView* fromSocketView,
		NodeSocketView* toSocketView, QGraphicsItem* parent = nullptr);
	// Contruction for temporary link 
	explicit NodeLinkView(const QPointF& startPosition,
		const QPointF& endPosition, QGraphicsItem* parent = nullptr,
		bool invertStartPos = false);
	virtual ~NodeLinkView();

	virtual void paint(QPainter *painter, 
		const QStyleOptionGraphicsItem* option, QWidget* widget);
	virtual int type() const;
	virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;

	// Uaktualnia ksztlat i pozycje na podstawie gniazd do ktorych wchodzi/wychodzi
	void updateFromSocketViews();
	void updateEndPosition(const QPointF& endPosition);

	// Zwraca true jesli obiekt laczy podane gniazda
	bool connects(NodeSocketView* from, NodeSocketView* to) const;
	// Zwraca true jesli obiekt wychodzi z podanego wezla 
	bool outputConnecting(NodeView* from) const;
	// Zwraca true jesli obiektu wchodzi do podanego wezla
	bool inputConnecting(NodeView* to) const;

	enum { Type = QGraphicsItem::UserType + 2 };

	void setDrawDebug(bool drawDebug);

	const NodeSocketView* fromSocketView() const;
	const NodeSocketView* toSocketView() const;

protected:
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:
	QPen mPen;
	QPointF mEndPosition;
	NodeSocketView* mFromSocketView;
	NodeSocketView* mToSocketView;
	QPainterPath mPath;
	mutable QPointF mc1, mc2;
	bool mDrawDebug;
	bool mInvertStartPos;

private:
	// Buduje i zwraca QPainterPath 
	QPainterPath shapeImpl() const;
};

inline int NodeLinkView::type() const
{ return NodeLinkView::Type; }

inline const NodeSocketView* NodeLinkView::fromSocketView() const
{ return mFromSocketView; }

inline const NodeSocketView* NodeLinkView::toSocketView() const
{ return mToSocketView; }
