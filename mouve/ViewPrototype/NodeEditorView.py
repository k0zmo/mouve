from PySide.QtCore import *
from PySide.QtGui import *

class NodeEditorView(QGraphicsView):
	def __init__(self, parent=None):
		QGraphicsView.__init__(self, parent)

		self._zoom = 1.0
		self.setRenderHint(QPainter.Antialiasing)
		self.scale(self._zoom, self._zoom)
		self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
		self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
		self.setFocusPolicy(Qt.ClickFocus)

		# !TODO: Make scroll wheel change it
		#self.setDragMode(QGraphicsView.ScrollHandDrag)
		self.setDragMode(QGraphicsView.RubberBandDrag)

	def wheelEvent(self, event):
		if event.orientation() == Qt.Vertical:
			self._zoom += event.delta() * 0.001
			self._zoom = min(max(self._zoom, 0.3), 3.0)
			self.setZoom(self._zoom)

	def setZoom(self, zoom):
		if self.scene() is not None:
			transformation = QTransform()
			transformation.scale(self._zoom, self._zoom)
			self.setTransform(transformation)

	def contextMenuEvent(self, event):
		self.contextMenu.emit(event.globalPos(), self.mapToScene(event.pos()))

	# Signals
	contextMenu = Signal(QPoint, QPointF)
