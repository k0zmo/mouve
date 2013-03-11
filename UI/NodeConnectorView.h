#pragma once

#include "Prerequisites.h"

#include <QGraphicsWidget>
#include <QPropertyAnimation>
#include <QPen>

#include "NodeSocketView.h"

class NodeConnectorView : public QGraphicsWidget
{
	Q_OBJECT
	Q_PROPERTY(float penWidth READ penWidth WRITE setPenWidth)
public:
	explicit NodeConnectorView(bool isOutput, QGraphicsItem* parent = nullptr);
	
	virtual void paint(QPainter* painter, 
		const QStyleOptionGraphicsItem* option, QWidget *widget);
	virtual QRectF boundingRect() const;
	/// xXx: virtual QPainterPath shape() const;
	virtual int type() const;

	void setBrushGradient(const QColor& start, const QColor& stop);
	float penWidth() const;
	void setPenWidth(float penWidth);

	void setHighlight(bool highlight);
	QPointF centerPos() const;
	NodeSocketView* socketView() const;

	bool isOutput() const;

	enum { Type = QGraphicsItem::UserType + 4 };

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
	QRect mRect;
	QPen mPen;
	QBrush mBrush;
	QPropertyAnimation mAnimation;
	NodeTemporaryLinkView* mTemporaryLink;
	NodeConnectorView* mHoveredConnector;
	bool mIsOutput;

private:
	NodeConnectorView* canDrop(const QPointF& scenePos);

signals:
	void draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*);
};

inline float NodeConnectorView::penWidth() const 
{ return mPen.widthF(); }

inline QRectF NodeConnectorView::boundingRect() const
{ return mRect; }

inline int NodeConnectorView::type() const
{ return NodeConnectorView::Type; }

inline bool NodeConnectorView::isOutput() const
{ return mIsOutput; }

inline QPointF NodeConnectorView::centerPos() const
{ return QPointF(mRect.width() * 0.5, mRect.height() * 0.5); }

inline NodeSocketView* NodeConnectorView::socketView() const
{ return static_cast<NodeSocketView*>(parentObject()); }
