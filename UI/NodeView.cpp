#include "NodeView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"
#include "Controller.h"

#include <QPainter>

NodeView::NodeView(const QString& title, QGraphicsItem* parent)
	: QGraphicsWidget(parent)
	, mLabel(new QGraphicsSimpleTextItem(this))
	, mDropShadowEffect(new QGraphicsDropShadowEffect(this))
	, mPreviewSelected(false)
{
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	// Node title
	mLabel->setText(title);
	mLabel->setFont(NodeStyle::NodeTitleFont);
	mLabel->setBrush(NodeStyle::NodeTitleFontBrush);

	// Additional visual effect
	mDropShadowEffect->setOffset(5.0, 5.0);
	mDropShadowEffect->setBlurRadius(12.0);
	mDropShadowEffect->setColor(QColor(0, 0, 0, 50));
	setGraphicsEffect(mDropShadowEffect);

	// To alter Z value on moouse hover
	setAcceptHoverEvents(true);

	// Rebuild the view layout
	updateLayout();
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
	const QString& title, bool isOutput)
{
	// Check if the socketKey hasn't been added already
	auto& views = isOutput
		? mOutputSocketViews 
		: mInputSocketViews;

	if(!views.contains(socketKey))
	{
		NodeSocketView* socketView = new NodeSocketView(title, isOutput, this);
		socketView->setData(NodeDataIndex::SocketKey, socketKey);

		connect(socketView, SIGNAL(draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*)),
			gC, SLOT(draggingLinkDrop(QGraphicsWidget*, QGraphicsWidget*)));
		connect(socketView, SIGNAL(draggingLinkStarted(QGraphicsWidget*)),
			gC, SLOT(draggingLinkStart(QGraphicsWidget*)));
		connect(socketView, SIGNAL(draggingLinkStopped(QGraphicsWidget*)),
			gC, SLOT(draggingLinkStop(QGraphicsWidget*)));

		views.insert(socketKey, socketView);

		updateLayout();
		return socketView;
	}
	else
	{
		qWarning() << "SocketView with socketKey =" << socketKey << "already exists";
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
	Q_UNUSED(event);
	setZValue(NodeStyle::ZValueNodeHovered);
}

void NodeView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	setZValue(NodeStyle::ZValueNode);
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
		+ 3 * NodeStyle::NodeTitleSize;

	// During first pass we layout input slots and calculate
	// required spacing between them and output slots.
	// Then, during second pass we set correct position of a node 
	// title label and output slots

	// Make some minimum size
	qreal totalWidth = qMax(qreal(10.0),
		qreal(titleWidth + 2 * NodeStyle::NodeTitleHorizontalMargin));

	qreal yPos = titleHeight;
	qreal inputsWidth = 0.0;
	qreal outputsWidth = 0.0;

#if 0
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
	qreal inputsHeight = qMax(yPos, titleHeight * 1.5); // if node is trivial

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
	qreal inputsHeight = qMax(yPos, titleHeight * 1.5); // if node is trivial
	yPos = titleHeight;

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
	mLabel->setPos((totalWidth - titleWidth) / 2.0,
		NodeStyle::NodeTitleSize);
	resize(totalWidth, qMax(yPos, inputsHeight));

	// Generate painter paths
	qreal yy = mLabel->boundingRect().bottom() + 2 * NodeStyle::NodeTitleSize;
	mShape1 = shape1(yy);
	mShape2 = shape2(yy);

	update();
}

void NodeView::setNodeViewName(const QString& newName)
{
	mLabel->setText(newName);
	updateLayout();
}