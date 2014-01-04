#pragma once

#include "../Prerequisites.h"

#include <QGraphicsWidget>
#include <QGraphicsDropShadowEffect>

class NodeView : public QGraphicsWidget
{
	Q_OBJECT
public:
	explicit NodeView(const QString& title, const QString& typeName,
		QGraphicsItem* parent = nullptr);
	virtual ~NodeView();

	virtual int type() const;
	virtual void paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option, QWidget* widget);

	NodeID nodeKey() const;
	NodeSocketView* addSocketView(SocketID socketKey, 
		ENodeFlowDataType dataType,	const QString& title,
		bool isOutput = false);
	void updateLayout();
	void selectPreview(bool selected);

	enum { Type = QGraphicsItem::UserType + 1 };

	void setNodeViewName(const QString& newName);

	NodeSocketView* inputSocketView(SocketID socketID) const;
	NodeSocketView* outputSocketView(SocketID socketID) const;

	int outputSocketCount() const;
	SocketID previewSocketID() const;
	void setPreviewSocketID(SocketID socketID);

	void setTimeInfo(const QString& text);
	void setTimeInfoVisible(bool visible);

	void setNodeWithStateMark(bool visible);

	bool isNodeEnabled() const;
	void setNodeEnabled(bool enable);

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);

private:
	QGraphicsSimpleTextItem* mLabel;
	QGraphicsSimpleTextItem* mTypeLabel;
	QGraphicsSimpleTextItem* mTimeInfo;
	QGraphicsItem* mStateMark;
	QGraphicsDropShadowEffect* mDropShadowEffect;
	QPainterPath mShape1;
	QPainterPath mShape2;
	QMap<SocketID, NodeSocketView*> mInputSocketViews;
	QMap<SocketID, NodeSocketView*> mOutputSocketViews;
	int mPreviewSocketID;
	bool mPreviewSelected;
	bool mStateMarkVisible;
	bool mNodeEnabled;

private:
	QPainterPath shape1(qreal titleHeight) const;
	QPainterPath shape2(qreal titleHeight) const;

signals:
	void mouseDoubleClicked(NodeView*);

	void mouseHoverEntered(NodeID nodeID, QGraphicsSceneHoverEvent* event);
	void mouseHoverLeft(NodeID nodeID, QGraphicsSceneHoverEvent* event);
};

inline int NodeView::type() const
{ return NodeView::Type; }

inline NodeID NodeView::nodeKey() const
{ return data(NodeDataIndex::NodeKey).toInt(); }

inline NodeSocketView* NodeView::inputSocketView(SocketID socketID) const
{ return mInputSocketViews.value(socketID); }

inline NodeSocketView* NodeView::outputSocketView(SocketID socketID) const
{ return mOutputSocketViews.value(socketID); }

inline int NodeView::outputSocketCount() const
{ return mOutputSocketViews.count(); }

inline SocketID NodeView::previewSocketID() const
{ return mPreviewSocketID; }

inline void NodeView::setPreviewSocketID(SocketID socketID)
{ mPreviewSocketID = socketID; }

inline bool NodeView::isNodeEnabled() const
{ return mNodeEnabled; }
