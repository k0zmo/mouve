#include "NodeSocketView.h"
#include "NodeStyle.h"
#include "NodeConnectorView.h"

NodeSocketView::NodeSocketView(const QString& title, bool isOutput,
	QGraphicsItem* parent)
	: QGraphicsWidget(parent)
	, mTitle(title)
	, mIsOutput(isOutput)
	, mLabel(new QGraphicsSimpleTextItem(this))
	, mConnector(new NodeConnectorView(isOutput, this))
{
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	mLabel->setBrush(NodeStyle::SocketTitleBrush);
	mLabel->setText(title);
	mLabel->setFont(NodeStyle::SocketFont);

	// self._links = []

	// Send the signals from connector elsewhere
	connect(mConnector, SIGNAL(draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*)),
		SIGNAL(draggingLinkDropped(QGraphicsWidget*, QGraphicsWidget*)));
	connect(mConnector, SIGNAL(draggingLinkStarted(QGraphicsWidget*)),
		SIGNAL(draggingLinkStarted(QGraphicsWidget*)));
	connect(mConnector, SIGNAL(draggingLinkStopped(QGraphicsWidget*)),
		SIGNAL(draggingLinkStopped(QGraphicsWidget*)));
}

void NodeSocketView::updateLayout()
{
	const QRectF& b = mLabel->boundingRect();
	const QRectF& bb = mConnector->boundingRect();
	qreal h = (b.height() - bb.height()) / 2;
	qreal height = qMax(bb.height(), b.height());
	qreal width = b.width() + qreal(3.0) + bb.width();

	if(isOutput())
	{
		mConnector->setPos(b.width() + 3.0, h);
		mLabel->setPos(0, mLabel->pos().y());
	}
	else
	{
		mConnector->setPos(0, h);
		mLabel->setPos(bb.width() + 3.0, mLabel->pos().y());
	}
	resize(width, height);
}

void NodeSocketView::setActive(bool active)
{
	mLabel->setBrush(active 
		? NodeStyle::SocketTitleBrush 
		: NodeStyle::SocketTitleInactiveBrush);
}

QPointF NodeSocketView::connectorCenterPos() const
{ 
	return mConnector->pos() + mConnector->centerPos();
}

QVariant NodeSocketView::itemChange(GraphicsItemChange change,
	const QVariant& value)
{
	// Update link views connecting this socket view
	if (change == QGraphicsItem::ItemScenePositionHasChanged)
	{
		//for linkView in self._links:
		//	linkView.updateFromSocketViews()
	}
	// Default implementation does nothing and return 'value'
	return value;
}

//def addLink(self, nodeLinkView):
//self._links.append(nodeLinkView)
//
//	def removeLink(self, nodeLinkView):
//self._links.remove(nodeLinkView)