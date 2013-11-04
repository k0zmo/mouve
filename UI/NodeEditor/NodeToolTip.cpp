#include "NodeToolTip.h"
#include "NodeStyle.h"

#include <QPainter>

NodeToolTip::NodeToolTip(QGraphicsItem* parent)
	: QGraphicsItem(parent)
	, _text()
	, _boundingRect(QRectF())
{
	setCacheMode(DeviceCoordinateCache);
	setZValue(300);
}

QRectF NodeToolTip::boundingRect() const
{
	return _boundingRect;
}

void NodeToolTip::paint(QPainter* painter, 
		   const QStyleOptionGraphicsItem* option,
		   QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	QRectF textRect = boundingRect().adjusted(10, 10, -10, -10);
	int flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap;

	painter->setPen(QPen(Qt::black, 2));
	painter->setBrush(QColor(245, 245, 255, 180));
	painter->setClipRect(boundingRect());
	painter->drawRect(boundingRect());

	painter->setPen(Qt::black);
	painter->setFont(NodeStyle::SocketFont);
	painter->drawText(textRect, flags, _text);

}

void NodeToolTip::setText(const QString& text)
{
	_text = text;

	// Calculate bounding rect
	QFontMetrics metrics(NodeStyle::SocketFont);
	QRect textRect = QRect(0, 0, 250, 1500);
	int flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap;
	_boundingRect = metrics.boundingRect(textRect, flags, text)
		.adjusted(-10, -10, 10, 10);

	update();
}
