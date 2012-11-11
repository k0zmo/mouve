#include "NodeView.h"
#include "NodeStyle.h"
#include "NodeSocketView.h"
#include <QGraphicsDropShadowEffect>

NodeView::NodeView(const QString& title, QGraphicsItem* parent)
	: QGraphicsWidget(parent)
	, mLabel(new QGraphicsSimpleTextItem(this))
{
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	// Node title
	mLabel->setText(title);
	mLabel->setFont(NodeStyle::NodeTitleFont);
	mLabel->setBrush(NodeStyle::NodeTitleFontBrush);

	// !TODO: set this based on model
	setToolTip("ToolTip");

	// Additional visual effect
	static QGraphicsDropShadowEffect dropShadowEffect;
	dropShadowEffect.setOffset(5.0, 5.0);
	dropShadowEffect.setBlurRadius(12.0);
	dropShadowEffect.setColor(QColor(0, 0, 0, 50));
	setGraphicsEffect(&dropShadowEffect);

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

NodeSocketView* NodeView::addSocketView(quint32 socketKey,
	const QString& title, bool isOutput)
{
	NodeSocketView* socketView = new NodeSocketView(title, isOutput, this);
	//socketView->setData(NodeDataIndex::SocketKey, socketKey)

	//socketView.draggingLinkDropped.connect(NodeController.controller.draggingLinkDropped);
	//socketView.draggingLinkStarted.connect(NodeController.controller.draggingLinkStarted);
	//socketView.draggingLinkStopped.connect(NodeController.controller.draggingLinkStopped);

	// Add socket view to the map
	auto& views = isOutput
		? mOutputSocketViews 
		: mInputSocketViews;
	views[socketKey] = socketView;

	updateLayout();
	return socketView;
}

void NodeView::paint(QPainter* painter, 
	const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setPen(isSelected()
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
}

void NodeView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	setZValue(NodeStyle::ZValueNode);
}

void NodeView::updateLayout()
{
	qreal titleWidth = mLabel->boundingRect().width();
	qreal titleHeight = mLabel->boundingRect().bottom() + 3 * NodeStyle::NodeTitleSize;

	// During first pass we layout input slots and calculate
	// required spacing between them and output slots.
	// Then, during second pass we set correct position of a node 
	// title label and output slots

	// Make some minimum size
	qreal totalWidth = qMax(qreal(10.0), qreal(titleWidth + 2 * NodeStyle::NodeTitleHorizontalMargin));

	qreal yPos = titleHeight;
	qreal inputsWidth = 0.0;
	qreal outputsWidth = 0.0;

	// First pass
	for(auto it = mInputSocketViews.begin(); it != mInputSocketViews.end(); ++it)
	{
		auto sv = it->second;
		if(!sv->isVisible())
			continue;
		QRectF b = sv->boundingRect();
		sv->updateLayout();
		sv->setPos(NodeStyle::NodeSocketHorizontalMargin, yPos);
		yPos += b.height() + NodeStyle::NodeSocketVerticalMargin;
		inputsWidth = qMax(inputsWidth,
			NodeStyle::NodeSocketHorizontalMargin + b.width());
	}
	for(auto it = mOutputSocketViews.begin(); it != mOutputSocketViews.end(); ++it)
	{
		auto sv = it->second;
		if(!sv->isVisible())
			continue;
		sv->updateLayout();
		outputsWidth = qMax(outputsWidth, 
			NodeStyle::NodeSocketHorizontalMargin + sv->boundingRect().width());
	}

	totalWidth = qMax(totalWidth, outputsWidth + inputsWidth + NodeStyle::NodeSocketsMargin);

	// Second pass
	qreal inputsHeight = qMax(yPos, titleHeight * 1.5); // if node is trivial
	yPos = titleHeight;

	for(auto it = mOutputSocketViews.begin(); it != mOutputSocketViews.end(); ++it)
	{
		auto sv = it->second;
		if(!sv->isVisible())
			continue;
		QRectF b = sv->boundingRect();
		sv->setPos(totalWidth - 
			(b.width() + NodeStyle::NodeSocketHorizontalMargin), yPos);
		yPos += b.height() + NodeStyle::NodeSocketVerticalMargin;
	}

	// Center title
	mLabel->setPos((totalWidth - titleWidth) / 2.0, NodeStyle::NodeTitleSize);
	resize(totalWidth, qMax(yPos, inputsHeight)); // Does it call prepareGeometryChange?

	// Generate painter paths
	qreal yy = mLabel->boundingRect().bottom() + 2 * NodeStyle::NodeTitleSize;
	mShape1 = shape1(yy);
	mShape2 = shape2(yy);

	update();
}
