from PySide.QtCore import *
from PySide.QtGui import *
from NodeStyle import *

class NodeLinkView(QGraphicsItem):
	def __init__(self, fromSocketView, toSocketView, parent=None):
		QGraphicsItem.__init__(self, parent)
		
		self._drawDebug = False
		self._drawMode = 1
		self._fromSocketView = fromSocketView
		self._toSocketView = toSocketView
		self._pen = QPen(NodeStyle.LinkPen)

		# !TODO: Dla usuwania wezlow potrzebna jest mozliwosc ich zaznaczenia
		#        Na razie jednak kolizja dziala w oparciu o boundingRect 
		#        co nie bardzo nas zadowola dla cienkich linii
		self.setFlag(QGraphicsItem.ItemIsSelectable)
		self.setFlag(QGraphicsItem.ItemIsFocusable)
		self.setZValue(NodeStyle.ZValueLink)

		self.updateFromSocketViews()

	def __del__(self):
		self.dtor()

	def itemChange(self, change, value):
		if change == QGraphicsItem.ItemSelectedHasChanged:
			if value:
				self._pen.setStyle(Qt.DashLine)
				self.update()
			else:
				self._pen.setStyle(Qt.SolidLine)
				self.update()
		return value

	def dtor(self):
		# Remove the references so we socket views won't point to no man's land
		if self._fromSocketView:
			self._fromSocketView.removeLink(self)
		if self._toSocketView:
			self._toSocketView.removeLink(self)

	def paint(self, painter, option, widget):
		painter.setPen(self._pen)
		painter.drawPath(self._shape())
		# Draw debugging red-ish rectangle
		if self._drawDebug:
			painter.setBrush(QColor(255,0,0,50))
			painter.setPen(Qt.red)
			painter.drawRect(self.boundingRect())

	def boundingRect(self):
		x1 = min(0.0, self._endPosition.x())
		y1 = min(0.0, self._endPosition.y())
		x2 = max(0.0, self._endPosition.x())
		y2 = max(0.0, self._endPosition.y())
		return QRectF(QPointF(x1, y1), QPointF(x2, y2))

	'''
	Uaktualnia ksztlat i pozycje na podstawie gniazd do ktorych wchodzi/wychodzi
	'''
	def updateFromSocketViews(self):
		if self._fromSocketView and self._toSocketView:
			# NOTE: Origin is always at 0,0 in item coordinates
			self.prepareGeometryChange()
			self.setPos(self._fromSocketView.scenePos() + self._fromSocketView.connectorCenterPos())
			self._endPosition = self.mapFromScene(self._toSocketView.scenePos() + self._toSocketView.connectorCenterPos())

	'''
	Zwraca true jesli obiekt laczy podane gniazda
	'''
	def connects(self, fromSocketView, toSocketView):
		return fromSocketView == self._fromSocketView and toSocketView == self._toSocketView

	'''
	 Zwraca true jesli obiekt wychodzi z podanego wezla 
	'''
	def outputConnecting(self, nodeView):
		return nodeView == self._fromSocketView.nodeView()

	'''
	Zwraca true jesli obiektu wchodzi do podanego wezla
	'''
	def inputConnecting(self, nodeView):
		return nodeView == self._toSocketView.nodeView()

	'''
	Zwraca true jesli obiekt wchodzi lub wychodzi do podanego wezla (ma z nim jakikolwiek styk)
	'''
	def connecting(self, nodeView):
		return self.outputConnecting(nodeView) or self.inputConnecting(nodeView)

	'''
	Buduje i zwraca QPainterPath 
	'''
	def _shape(self):
		endPos = self._endPosition

		# Default bezier drawing NodeStyle
		if self._drawMode == 1:
			c1 = QPointF(endPos.x() / 2.0, 0)
			c2 = QPointF(endPos.x() / 2.0, endPos.y())

			painterPath = QPainterPath()
			painterPath.cubicTo(c1, c2, endPos)
			return painterPath

		#
		# Not quite working ;p 
		#
		elif self._drawMode == 2:
			tangentLength = (abs(endPos.x()) / 2.0) + (abs(endPos.y()) / 4.0)
			startTangent = QPointF(tangentLength, 0.0)
			endTangent = QPointF(endPos.x() - tangentLength, endPos.y())

			painterPath = QPainterPath()
			painterPath.cubicTo(startTangent, endTangent, endPos)
			return painterPath

		# Prosta linia
		else:
			painterPath = QPainterPath(QPointF(0, 0))
			painterPath.lineTo(endPos)
			return painterPath

	''' 
	Zmiana trybu rysowania krzywej
	'''
	@Slot(int)
	def setDrawMode(self, drawMode):
		self._drawMode = drawMode
		self.update()

	@Slot(bool)
	def setDrawDebug(self, drawDebug):
		self._drawDebug = drawDebug
		self.update()

	#
	# type property
	#
	
	Type = QGraphicsItem.UserType + 2

	def type(self):
		return NodeLinkView.Type