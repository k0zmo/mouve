from PySide.QtCore import *
from PySide.QtGui import *
from NodeStyle import *
from NodeConnectorView import *
from NodeDataIndex import *

class NodeSocketView(QGraphicsWidget):
	def __init__(self, title, isOutput, parent=None):
		QGraphicsWidget.__init__(self, parent)

		self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges)
		self.isOutput = isOutput
		self.title = title

		self._label = QGraphicsSimpleTextItem(self)
		self._label.setBrush(NodeStyle.SocketTitleBrush)
		self._label.setText(title)
		self._label.setFont(NodeStyle.SocketFont)

		self._links = []

		self._connector = NodeConnectorView(isOutput, self)
		# Send the signals from connector elsewhere
		self._connector.draggingLinkDropped.connect(self.draggingLinkDropped)
		self._connector.draggingLinkStarted.connect(self.draggingLinkStarted)
		self._connector.draggingLinkStopped.connect(self.draggingLinkStopped)

	def itemChange(self, change, value):
		# Update link views connecting this socket view
		if change == QGraphicsItem.ItemScenePositionHasChanged:
			for linkView in self._links:
				linkView.updateFromSocketViews()
		# The default implementation does nothing and return 'value'
		return value

	def addLink(self, nodeLinkView):
		self._links.append(nodeLinkView)

	def removeLink(self, nodeLinkView):
		self._links.remove(nodeLinkView)

	'''
	Przelicza i ustawia rozmiary i pozycje nazwy oraz "wtyczki"
	'''
	def updateLayout(self):
		b = self._label.boundingRect()
		bb = self._connector.boundingRect()
		h = (b.height() - bb.height()) / 2
		height = max(bb.height(), b.height())
		width = b.width() + 3.0 + bb.width()

		if self.isOutput:
			self._connector.setPos(b.width() + 3.0, h)
			self._label.setPos(0, self._label.pos().y())
		else:
			self._connector.setPos(0, h)
			self._label.setPos(bb.width() + 3.0, self._label.pos().y())

		self.resize(width, height)

	'''
	Zmienia aktywnosc gniazda
	'''
	def setActive(self, active):
		self._label.setBrush(NodeStyle.SocketTitleBrush if active 
			else NodeStyle.SocketTitleInactiveBrush)

	'''
	Zwraca widok wezla do ktorego nalezy gniazdo
	'''
	def nodeView(self):
		return self.parentObject()

	'''
	Zwraca klucz gniazda
	'''
	def socketKey(self):
		return self.data(NodeDataIndex.SocketKey)

	'''
	Zwraca pozycje srodka widoku gniazda
	'''
	def connectorCenterPos(self):
		return self._connector.pos() + self._connector.centerPos()

	#
	# type property
	#

	Type = QGraphicsItem.UserType + 3

	def type(self):
		return NodeSocketView.Type

	# signals

	draggingLinkDropped = Signal(QGraphicsWidget, QGraphicsWidget)
	draggingLinkStarted = Signal(QGraphicsWidget)
	draggingLinkStopped = Signal(QGraphicsWidget)