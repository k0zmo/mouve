#pragma once

#include "Prerequisites.h"

#include <QGraphicsWidget>
#include <QGraphicsSimpleTextItem>

class NodeSocketView : public QGraphicsWidget
{
	Q_OBJECT
public:
	explicit NodeSocketView(const QString& title,
		bool isOutput, QGraphicsItem* parent = nullptr);
	virtual int type() const;

	// Przelicza i ustawia rozmiary i pozycje nazwy oraz "wtyczki"
	void updateLayout();

	// Zmienia aktywnosc gniazda
	void setActive(bool active);

	// Zwraca widok wezla do ktorego nalezy gniazdo
	NodeView* nodeView() const;

	// Zwraca klucz gniazda
	SocketID socketKey() const;

	// Zwraca pozycje srodka widoku gniazda
	QPointF connectorCenterPos() const;

	bool isOutput() const;
	const QString& title() const;

	void addLink(NodeLinkView* link);
	void removeLink(NodeLinkView* link);

	enum { Type = QGraphicsItem::UserType + 3 };

protected:
	virtual QVariant itemChange(GraphicsItemChange change,
		const QVariant& value);

private:
	QString mTitle;
	bool mIsOutput;

	QGraphicsSimpleTextItem* mLabel;
	NodeConnectorView* mConnector;
	QList<NodeLinkView*> mLinks;

signals:
	void draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*);
	void draggingLinkStarted(QGraphicsWidget*);
	void draggingLinkStopped(QGraphicsWidget*);
};

inline int NodeSocketView::type() const
{ return NodeSocketView::Type; }

inline bool NodeSocketView::isOutput() const
{ return mIsOutput; }

inline const QString& NodeSocketView::title() const
{ return mTitle; }

inline SocketID NodeSocketView::socketKey() const
{ return data(NodeDataIndex::SocketKey).toInt(); }
