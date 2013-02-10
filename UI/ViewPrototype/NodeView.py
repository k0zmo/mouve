from PySide.QtCore import *
from PySide.QtGui import *
from NodeStyle import *
from NodeEditorView import *
from NodeSocketView import *
from NodeDataIndex import *
import NodeController 

class NodeView(QGraphicsWidget):
	def __init__(self, title, parent=None):
		QGraphicsWidget.__init__(self, parent)

		self.setFlag(QGraphicsItem.ItemIsMovable)
		self.setFlag(QGraphicsItem.ItemIsSelectable)
		self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges)

		# Node title
		self.label = QGraphicsSimpleTextItem(self)
		self.label.setText(title)
		self.label.setFont(NodeStyle.NodeTitleFont)
		self.label.setBrush(NodeStyle.NodeTitleFontBrush)

		# TODO: set this based on model
		self.setToolTip('ToolTip')

		# Initialize node vide with empty socket views
		self._inputSocketViews = {}
		self._outputSocketViews = {}

		# Additional visual effect
		self._dropShadowEffect = QGraphicsDropShadowEffect()
		self._dropShadowEffect.setOffset(5.0, 5.0)
		self._dropShadowEffect.setBlurRadius(12.0)
		self._dropShadowEffect.setColor(QColor(0, 0, 0, 50))
		self.setGraphicsEffect(self._dropShadowEffect)
		
		# To alter Z value on moouse hover
		self.setAcceptHoverEvents(True)

		# Rebuild the view layout
		self._updateLayout()

	def hoverEnterEvent(self, event):
		self.setZValue(NodeStyle.ZValueNodeHovered)

	def hoverLeaveEvent(self, event):
		self.setZValue(NodeStyle.ZValueNode)

	def itemChange(self, change, value):
		# if change == QGraphicsItem.ItemSelectedHasChanged:
		# 	pass
		# The default implementation does nothing and return 'value'
		return value

	# def contextMenuEvent(self, event):
	# 	print 'context'

	# def mousePressEvent(self, event):
	# 	if event.button() is Qt.RightButton:
	# 		# TODO:
	# 		# delete node
	# 		print 'qwe'
	# 	else:
	# 		QGraphicsWidget.mousePressEvent(self, event)

	# def mouseReleaseEvent(self, event):
	# 	QGraphicsWidget.mouseReleaseEvent(self, event)

	# def mouseMoveEvent(self, event):
	# 	QGraphicsWidget.mouseMoveEvent(self, event)

	# def mouseDoubleClickEvent(self, event):
	# 	QGraphicsWidget.mouseDoubleClickEvent(self, event)

	def paint(self, painter, option, widget):

		painter.setPen(NodeStyle.NodeBorderSelectedPen if self.isSelected() else NodeStyle.NodeBorderPen)
		painter.setBrush(NodeStyle.NodeTitleBrush)
		painter.drawPath(self._shape11)

		painter.setBrush(NodeStyle.NodeBrush)
		painter.drawPath(self._shape22)

	def addSocketView(self, key, name, isOutput=False):
		socketView = NodeSocketView(name, isOutput, self)
		socketView.setData(NodeDataIndex.SocketKey, key)

		socketView.draggingLinkDropped.connect(NodeController.controller.draggingLinkDropped)
		socketView.draggingLinkStarted.connect(NodeController.controller.draggingLinkStarted)
		socketView.draggingLinkStopped.connect(NodeController.controller.draggingLinkStopped)

		# Add socket view to the dict
		views = self._outputSocketViews if isOutput else self._inputSocketViews
		views[key] = socketView

		self._updateLayout()

		return socketView

	def nodeKey(self):
		return self.data(NodeDataIndex.NodeKey)

	def _updateLayout(self):	
		titleWidth = self.label.boundingRect().width()
		titleHeight = self.label.boundingRect().bottom() + 3 * NodeStyle.NodeTitleSize

		# During first pass we layout input slots and calculate
		# required spacing between them and output slots.
		# Then, during second pass we set correct position of a node 
		# title label and output slots

		# Make some minimum size
		totalWidth = max(10.0, titleWidth + 2 * NodeStyle.NodeTitleHorizontalMargin) 

		yPos = titleHeight
		inputsWidth = 0.0
		outputsWidth = 0.0

		# First pass
		for key, sv in self._inputSocketViews.iteritems():
			if sv.isVisible == False:
				continue

			sv.updateLayout()
			sv.setPos(NodeStyle.NodeSocketHorizontalMargin, yPos)
			yPos += sv.boundingRect().height() + NodeStyle.NodeSocketVerticalMargin
			inputsWidth = max(NodeStyle.NodeSocketHorizontalMargin + sv.boundingRect().width(), inputsWidth)

		for key, sv in self._outputSocketViews.iteritems():
			if sv.isVisible == False:
				continue

			sv.updateLayout()
			outputsWidth = max(NodeStyle.NodeSocketHorizontalMargin + sv.boundingRect().width(), outputsWidth)

		totalWidth = max(totalWidth, outputsWidth + inputsWidth + NodeStyle.NodeSocketsMargin)

		# Second pass
		inputsHeight = max(yPos, titleHeight * 1.5) # if node is trivial
		yPos = titleHeight

		for key, sv in self._outputSocketViews.iteritems():
			if sv.isVisible == False:
				continue

			sv.setPos(totalWidth - (sv.boundingRect().width() + NodeStyle.NodeSocketHorizontalMargin), yPos)
			yPos += sv.boundingRect().height() + NodeStyle.NodeSocketVerticalMargin

		# Center title
		self.label.setPos((totalWidth - titleWidth) / 2.0, NodeStyle.NodeTitleSize)
		self.resize(totalWidth, max(yPos, inputsHeight)) # Does it call prepareGeometryChange?

		# Generate painter paths
		yy = self.label.boundingRect().bottom() + 2 * NodeStyle.NodeTitleSize
		self._shape11 = self._shape1(yy)
		self._shape22 = self._shape2(yy)

		self.update()

	def _shape1(self, titleHeight):
		w = self.rect().width()
		x = self.rect().x()
		y = self.rect().y()
		arc = NodeStyle.NodeRoundArc

		shape = QPainterPath()
		shape.moveTo(w, y+titleHeight)
		shape.lineTo(w, y+arc)
		shape.arcTo(w-arc, y, arc, arc, 0, 90)
		shape.lineTo(x+arc, y)
		shape.arcTo(x, y, arc, arc, 90, 90)
		shape.lineTo(x, y+titleHeight)
		return shape

	def _shape2(self, titleHeight):
		w = self.rect().width()
		h = self.rect().height()
		x = self.rect().x()
		y = self.rect().y()
		arc = NodeStyle.NodeRoundArc

		shape = QPainterPath()
		shape.moveTo(x, y+titleHeight)
		shape.lineTo(x, h-arc)
		shape.arcTo(x, h-arc, arc, arc, 180, 90)
		shape.lineTo(w-arc, h)
		shape.arcTo(w-arc, h-arc, arc, arc, 270, 90)
		shape.lineTo(w, y+titleHeight)
		return shape

	#
	# type property
	#

	Type = QGraphicsItem.UserType + 1

	def type(self):
		return NodeView.Type

	#
	# Signals
	#

	#rightClicked