#pragma once

#include "Prerequisites.h"

class NodeView : public QGraphicsWidget
{
public:
	NodeView(const QString& title, QGraphicsItem* parent = nullptr);

	virtual int type() const;
	virtual void paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option, QWidget* widget);

	//NodeId nodeKey() const
	//{ return data(NodeDataIndex::NodeKey) }

	NodeSocketView* addSocketView(quint32 socketKey, 
		const QString& title, bool isOutput = false);

	void updateLayout();

	enum 
	{
		Type = QGraphicsItem::UserType + 1
	};

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

private:
	QGraphicsSimpleTextItem* mLabel;
	unordered_map<quint32, NodeSocketView*> mInputSocketViews;
	unordered_map<quint32, NodeSocketView*> mOutputSocketViews;
	QPainterPath mShape1;
	QPainterPath mShape2;

private:
	QPainterPath shape1(qreal titleHeight) const;
	QPainterPath shape2(qreal titleHeight) const;
};

inline int NodeView::type() const
{ return NodeView::Type; }