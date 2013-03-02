#pragma once

#include "Prerequisites.h"

#include <QGraphicsWidget>
#include <QGraphicsDropShadowEffect>

class NodeView : public QGraphicsWidget
{
	Q_OBJECT
public:
	explicit NodeView(const QString& title, QGraphicsItem* parent = nullptr);

	virtual int type() const;
	virtual void paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option, QWidget* widget);

	NodeID nodeKey() const;
	NodeSocketView* addSocketView(SocketID socketKey,
		const QString& title, bool isOutput = false);
	void updateLayout();
	void selectPreview(bool selected);

	enum { Type = QGraphicsItem::UserType + 1 };

	void setNodeViewName(const QString& newName);

	NodeSocketView* inputSocketView(SocketID socketID) const;
	NodeSocketView* outputSocketView(SocketID socketID) const;

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);

private:
	QGraphicsSimpleTextItem* mLabel;
	QGraphicsDropShadowEffect* mDropShadowEffect;
	QPainterPath mShape1;
	QPainterPath mShape2;
	QMap<SocketID, NodeSocketView*> mInputSocketViews;
	QMap<SocketID, NodeSocketView*> mOutputSocketViews;
	bool mPreviewSelected;

private:
	QPainterPath shape1(qreal titleHeight) const;
	QPainterPath shape2(qreal titleHeight) const;

signals:
	void mouseDoubleClicked(NodeView*);
};

inline int NodeView::type() const
{ return NodeView::Type; }

inline NodeID NodeView::nodeKey() const
{ return data(NodeDataIndex::NodeKey).toInt(); }

inline NodeSocketView* NodeView::inputSocketView(SocketID socketID) const
{ return mInputSocketViews.value(socketID); }

inline NodeSocketView* NodeView::outputSocketView(SocketID socketID) const
{ return mOutputSocketViews.value(socketID); }
