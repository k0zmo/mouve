from PySide.QtCore import *
from PySide.QtGui import *
from NodeView import *
from NodeLinkView import *
from NodeDataIndex import *
from NodeConnectorView import *

class NodeScene(QGraphicsScene):

	dragging = False
	hovered = None

	def __init__(self, parent=None):
		QGraphicsScene.__init__(self, parent)
		self.setBackgroundBrush(NodeStyle.SceneBackground)

	def mouseMoveEvent(self, event):
		if self.dragging:
			item = self.itemAt(event.scenePos())
			if item and item.type() == NodeConnectorView.Type:
				self.hovered = item
				self.hovered.setHighlight(True)				
			elif self.hovered:
				self.hovered.setHighlight(False)
				self.hovered = None
		QGraphicsScene.mouseMoveEvent(self, event)