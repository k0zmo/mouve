#include "NodeView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"

#include "Logic/NodeFlowData.h"

#include <QPainter>
#include <QDebug>

NodeView::NodeView(const QString& title, 
				   const QString& typeName,
				   QGraphicsItem* parent)
	: QGraphicsWidget(parent)
	, mLabel(new QGraphicsSimpleTextItem(this))
	, mTypeLabel(new QGraphicsSimpleTextItem(this))
	, mTimeInfo(new QGraphicsSimpleTextItem(this))
	, mDropShadowEffect(new QGraphicsDropShadowEffect(this))
	, mPreviewSocketID(0)
	, mPreviewSelected(false)
{
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	setZValue(NodeStyle::ZValueNode);

	// Node title
	mLabel->setText(title);
	mLabel->setFont(NodeStyle::NodeTitleFont);
	mLabel->setBrush(NodeStyle::NodeTitleFontBrush);

	// Node type name
	mTypeLabel->setText(typeName);
	mTypeLabel->setFont(NodeStyle::NodeTypeNameFont);
	mTypeLabel->setBrush(NodeStyle::NodeTypeNameFontBrush);

	// Additional visual effect
	mDropShadowEffect->setOffset(5.0, 5.0);
	mDropShadowEffect->setBlurRadius(12.0);
	mDropShadowEffect->setColor(QColor(0, 0, 0, 50));
	setGraphicsEffect(mDropShadowEffect);

	// Node time info
	mTimeInfo->setVisible(false);
	mTimeInfo->setText("0 ms");
	mTimeInfo->setFont(NodeStyle::NodeTimeInfoFont);
	mTimeInfo->setBrush(NodeStyle::NodeTimeInfoFontBrush);

	// To alter Z value on moouse hover
	setAcceptHoverEvents(true);

	// Rebuild the view layout
	updateLayout();
}

NodeView::~NodeView()
{
}

QPainterPath NodeView::shape1(qreal titleHeight) const
{
	qreal w = rect().width();
	qreal x = rect().x();
	qreal y = rect().y();
	int arc = NodeStyle::NodeRoundArc;

	QPainterPath shape;
	shape.moveTo(w, y+titleHeight);
	shape.lineTo(w, y+arc);
	shape.arcTo(w-arc, y, arc, arc, 0, 90);
	shape.lineTo(x+arc, y);
	shape.arcTo(x, y, arc, arc, 90, 90);
	shape.lineTo(x, y+titleHeight);
	return shape;
}

QPainterPath NodeView::shape2(qreal titleHeight) const
{
	qreal w = rect().width();
	qreal h = rect().height();
	qreal x = rect().x();
	qreal y = rect().y();
	int arc = NodeStyle::NodeRoundArc;

	QPainterPath shape;
	shape.moveTo(x, y+titleHeight);
	shape.lineTo(x, h-arc);
	shape.arcTo(x, h-arc, arc, arc, 180, 90);
	shape.lineTo(w-arc, h);
	shape.arcTo(w-arc, h-arc, arc, arc, 270, 90);
	shape.lineTo(w, y+titleHeight);
	return shape;
}

NodeSocketView* NodeView::addSocketView(SocketID socketKey, 
										ENodeFlowDataType dataType, 
										const QString& title,
										bool isOutput)
{
	// Check if the socketKey hasn't been added already
	auto& views = isOutput
		? mOutputSocketViews 
		: mInputSocketViews;

	if(!views.contains(socketKey))
	{
		NodeSocketView* socketView = new NodeSocketView(title, isOutput, this);
		socketView->setData(NodeDataIndex::SocketKey, socketKey);

		switch(dataType)
		{
		case ENodeFlowDataType::Image:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart1,
				NodeStyle::SocketGradientStop1);
			socketView->setConnectorToolTip("[Image]");
			break;
		case ENodeFlowDataType::ImageRgb:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart2,
				NodeStyle::SocketGradientStop2);
			socketView->setConnectorToolTip("[ImageRGB]");
			break;
		case ENodeFlowDataType::Array:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart3,
				NodeStyle::SocketGradientStop3);
			socketView->setConnectorToolTip("[Array]");
			break;
		case ENodeFlowDataType::Keypoints:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart4,
				NodeStyle::SocketGradientStop4);
			socketView->setConnectorToolTip("[Keypoints]");
			break;
		case ENodeFlowDataType::Matches:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart5,
				NodeStyle::SocketGradientStop5);
			socketView->setConnectorToolTip("[Matches]");
			break;
#if defined(HAVE_OPENCL)
		case ENodeFlowDataType::DeviceImage:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart1,
				NodeStyle::SocketGradientStop1);
			socketView->setConnectorToolTip("[DeviceImage]");
			socketView->setConnectorAnnotation(QStringLiteral("D"));
			break;
		case ENodeFlowDataType::DeviceArray:
			socketView->setConnectorBrushGradient(NodeStyle::SocketGradientStart3,
				NodeStyle::SocketGradientStop3);
			socketView->setConnectorToolTip("[DeviceArray]");
			socketView->setConnectorAnnotation(QStringLiteral("D"));
			break;
#endif
		case ENodeFlowDataType::Invalid:
		default:
			socketView->setConnectorBrushGradient(Qt::white, Qt::black);
			socketView->setConnectorToolTip("[Invalid]");
			break;
		}

		views.insert(socketKey, socketView);

		updateLayout();
		return socketView;
	}
	else
	{
		qCritical() << "SocketView with socketKey =" << socketKey << "already exists";
		return nullptr;
	}
}

void NodeView::paint(QPainter* painter, 
	const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setPen(mPreviewSelected
		? NodeStyle::NodeBorderPreviewSelectedPen 
		: isSelected()
		? NodeStyle::NodeBorderSelectedPen
		: NodeStyle::NodeBorderPen);
	painter->setBrush(NodeStyle::NodeTitleBrush);
	painter->drawPath(mShape1);

	painter->setBrush(NodeStyle::NodeBrush);
	painter->drawPath(mShape2);
}

void NodeView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	setZValue(NodeStyle::ZValueNodeHovered);
	emit mouseHoverEntered(nodeKey(), event);
}

void NodeView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	setZValue(NodeStyle::ZValueNode);
	emit mouseHoverLeft(nodeKey(), event);
}

void NodeView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	Q_UNUSED(event);
	emit mouseDoubleClicked(this);
}

void NodeView::selectPreview(bool selected)
{
	mPreviewSelected = selected;
	update();
}

void NodeView::updateLayout()
{
	qreal titleWidth = mLabel->boundingRect().width();
	qreal titleHeight = mLabel->boundingRect().bottom()
		+ 2 * NodeStyle::NodeTitleSize;

	qreal typeNameWidth = mTypeLabel->boundingRect().width();
	qreal typeNameHeight = mTypeLabel->boundingRect().bottom()
		+ 2 * NodeStyle::NodeTitleSize;

	// During first pass we layout input slots and calculate
	// required spacing between them and output slots.
	// Then, during second pass we set correct position of a node 
	// title label and output slots

	// Make some minimum size
	qreal totalWidth = qMax(qreal(10.0),
		qMax(qreal(titleWidth + 2 * NodeStyle::NodeTitleHorizontalMargin),
			qreal(typeNameWidth + 2 * NodeStyle::NodeTypeNameHorizontalMargin)));

	qreal yPos = titleHeight + typeNameHeight;
	const qreal yPosStart = yPos;
	qreal inputsWidth = 0.0;
	qreal outputsWidth = 0.0;

#if 1
	foreach(NodeSocketView* sv, mInputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		sv->updateLayout();
		sv->setPos(NodeStyle::NodeSocketHorizontalMargin, yPos);
		yPos += sv->boundingRect().height() + NodeStyle::NodeSocketVerticalMargin;
		inputsWidth = qMax(inputsWidth,
			NodeStyle::NodeSocketHorizontalMargin + sv->boundingRect().width() + 5.0);
	}

	foreach(NodeSocketView* sv, mOutputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		sv->updateLayout();
		outputsWidth = qMax(outputsWidth, 
			NodeStyle::NodeSocketHorizontalMargin + sv->boundingRect().width() + 5.0);
	}

	totalWidth = qMax(totalWidth, qMax(
		outputsWidth, inputsWidth));

	// Second pass
	qreal inputsHeight = qMax(yPos, yPosStart * 1.5); // if node is trivial

	foreach(NodeSocketView* sv, mOutputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		QRectF b = sv->boundingRect();
		sv->setPos(totalWidth - 
			(b.width() + NodeStyle::NodeSocketHorizontalMargin), yPos);
		yPos += b.height() + NodeStyle::NodeSocketVerticalMargin;
	}
#else
	// First pass
	foreach(NodeSocketView* sv, mInputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		sv->updateLayout();
		sv->setPos(NodeStyle::NodeSocketHorizontalMargin, yPos);
		yPos += sv->boundingRect().height() + NodeStyle::NodeSocketVerticalMargin;
		inputsWidth = qMax(inputsWidth,
			NodeStyle::NodeSocketHorizontalMargin + sv->boundingRect().width());
	}

	foreach(NodeSocketView* sv, mOutputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		sv->updateLayout();
		outputsWidth = qMax(outputsWidth, 
			NodeStyle::NodeSocketHorizontalMargin + sv->boundingRect().width());
	}

	totalWidth = qMax(totalWidth, 
		outputsWidth + inputsWidth + NodeStyle::NodeSocketsMargin);

	// Second pass
	qreal inputsHeight = qMax(yPos, yPosStart * 1.5); // if node is trivial
	yPos = yPosStart;

	foreach(NodeSocketView* sv, mOutputSocketViews.values())
	{
		if(!sv->isVisible())
			continue;
		QRectF b = sv->boundingRect();
		sv->setPos(totalWidth - 
			(b.width() + NodeStyle::NodeSocketHorizontalMargin), yPos);
		yPos += b.height() + NodeStyle::NodeSocketVerticalMargin;
	}
#endif
	// Center title
	mLabel->setPos((totalWidth - titleWidth) * 0.5,
		NodeStyle::NodeTitleSize);
	mTypeLabel->setPos((totalWidth - typeNameWidth) * 0.5,
		titleHeight);

	resize(totalWidth, qMax(yPos, inputsHeight));

	// Generate painter paths
	qreal yy = yPosStart - NodeStyle::NodeTitleSize;
	mShape1 = shape1(yy);
	mShape2 = shape2(yy);

	// Update time info
	mTimeInfo->setPos(0, size().height()+3);

	update();
}

void NodeView::setTimeInfo(const QString& text)
{
	mTimeInfo->setText(text);
	update();
}

void NodeView::setTimeInfoVisible(bool visible)
{
	mTimeInfo->setVisible(visible);
	update();
}

void NodeView::setNodeViewName(const QString& newName)
{
	mLabel->setText(newName);
	updateLayout();
}
