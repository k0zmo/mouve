#pragma once

#include <QGraphicsItem>

class NodeToolTip : public QGraphicsItem
{
public:
	explicit NodeToolTip(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const;
	void paint(QPainter* painter, 
		const QStyleOptionGraphicsItem* option,
		QWidget* widget = nullptr);
	void setText(const QString& text);

private:
	QString _text;
	QRectF _boundingRect;
};