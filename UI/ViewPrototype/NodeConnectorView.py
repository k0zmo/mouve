from PySide.QtCore import *
from PySide.QtGui import *
from NodeTemporaryLinkView import *

class NodeConnectorView(QGraphicsWidget):
	def __init__(self, isOutput, parent=None):
		QGraphicsWidget.__init__(self, parent)

		self.isOutput = isOutput

		self._templink = None
		self._rect = NodeStyle.SocketSize
		self._pen = QPen(NodeStyle.SocketPen)

		gradient = QLinearGradient(0, self._rect.y(), 0, self._rect.height())
		gradient.setColorAt(0, NodeStyle.SocketGradientStart)
		gradient.setColorAt(1, NodeStyle.SocketGradientStop)
		self._brush = QBrush(gradient)		

		self._penWidth = 1.0 # !TODO: Use style
		self.setAcceptHoverEvents(True)

		self._animation = QPropertyAnimation(self, "penWidth")
		self._animation.setDuration(250)
		self._animation.setEasingCurve(QEasingCurve.InOutQuad);
		self._animation.setStartValue(self._penWidth)

	def paint(self, painter, option, widget):
		painter.setBrush(self._brush)
		painter.setPen(self._pen)
		painter.drawEllipse(self._rect)

	def boundingRect(self):
		return self._rect

	def hoverEnterEvent(self, event):
		self.setHighlight(True)

	def hoverLeaveEvent(self, event):
		self.setHighlight(False)

	def mousePressEvent(self, event):
		if event.button() == Qt.LeftButton:
			if self.isOutput:# !TODO: or self.isOutput == False:
				#self._templink = NodeTemporaryLinkView(self.mapToScene(self.centerPos()), event.scenePos(), None)
				#self.scene().addItem(self._templink)
				self._templink = NodeTemporaryLinkView(self.centerPos(), event.scenePos(), self)
				self.draggingLinkStarted.emit(self.socketView())

	def mouseMoveEvent(self, event):
		if self._templink is not None:
			self._templink.updateEndPosition(event.scenePos())

	def mouseReleaseEvent(self, event):
		if self._templink is not None:
			self.draggingLinkStopped.emit(self.socketView())
			itemColliding = self._canDrop(event.scenePos())
			if itemColliding is not None:
				self.draggingLinkDropped.emit(self.socketView(), itemColliding.socketView())
			self.scene().removeItem(self._templink)
			self._templink = None

	###############################################################################################

	def setHighlight(self, hightlight):
		if hightlight:
			self._animation.setStartValue(self._animation.currentValue())
			self._animation.setEndValue(2.0) # !TODO: Use style
			self._animation.start()
		else:
			self._animation.setStartValue(self._animation.currentValue())
			self._animation.setEndValue(1.0) # !TODO: Use style
			self._animation.start()

	def centerPos(self):
		return QPointF(self._rect.width() * 0.5, self._rect.height() * 0.5)

	def socketView(self):
		return self.parentObject()

	def _canDrop(self, scenePos):
		items = self.scene().items(scenePos, Qt.IntersectsItemShape, Qt.DescendingOrder)
		for item in items:
			# Did we dropped on another connector
			if item.type() == NodeConnectorView.Type and item != self:
				# Ensure we connect from input to output or the opposite
				if item.isOutput != self.isOutput:
					# Protect from constructing algebraic loops
					if item.socketView().nodeView() != self.socketView().nodeView():
						return item
		return None

	#
	# type property
	#

	Type = QGraphicsItem.UserType + 4

	def type(self):
		return NodeConnectorView.Type

	# penWidth property for mouse hover animation

	def getPenWidth(self):
		return self._penWidth

	def setPenWidth(self, val):
		self._penWidth = val
		self._pen.setWidthF(self._penWidth)
		self.update()

	penWidth = Property(float, getPenWidth, setPenWidth)

	# signals
	draggingLinkDropped = Signal(QGraphicsWidget, QGraphicsWidget)
	draggingLinkStarted = Signal(QGraphicsWidget)
	draggingLinkStopped = Signal(QGraphicsWidget)

