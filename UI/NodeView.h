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
	NodeSocketView* addSocketView(NodeID socketKey,
		const QString& title, bool isOutput = false);
	void updateLayout();
	void selectPreview(bool selected);

	enum { Type = QGraphicsItem::UserType + 1 };

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);

private:
	QGraphicsSimpleTextItem* mLabel;
	QGraphicsDropShadowEffect* mDropShadowEffect;
	QPainterPath mShape1;
	QPainterPath mShape2;
	QHash<NodeID, NodeSocketView*> mInputSocketViews;
	QHash<NodeID, NodeSocketView*> mOutputSocketViews;
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
