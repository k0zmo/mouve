from PySide.QtCore import *
from PySide.QtGui import *
from NodeStyle import *

'''
Used when user is dragging from one socket to another
'''
class NodeTemporaryLinkView(QGraphicsItem):
	def __init__(self, startPosition, endPosition, parent=None):
		QGraphicsItem.__init__(self, parent)

		self._drawDebug = False
		self._drawMode = 1
		self._pen = QPen(NodeStyle.LinkPen)

		# Origin is always at 0,0 in item coordinates
		self.setPos(startPosition)
		self.setZValue(NodeStyle.ZValueTemporaryLink)
		self._endPosition = self.mapFromScene(endPosition)

	def paint(self, painter, option, widget):
		painter.setPen(self._pen)
		painter.drawPath(self._shape())
		# Draw debugging red-ish rectangle
		if self._drawDebug:
			painter.setBrush(Qt.transparent)
			painter.setPen(Qt.red)
			painter.drawRect(self.boundingRect())

	def boundingRect(self):
		# !TODO: zbadac to
		d = 0.0
		x1 = min(0.0, self._endPosition.x()) - d
		y1 = min(0.0, self._endPosition.y()) - d
		x2 = max(0.0, self._endPosition.x()) + d
		y2 = max(0.0, self._endPosition.y()) + d
		return QRectF(QPointF(x1, y1), QPointF(x2, y2))

	@Slot(int)
	def setDrawMode(self, drawMode):
		self._drawMode = drawMode
		self.update()

	@Slot(bool)
	def setDrawDebug(self, drawDebug):
		self._drawDebug = drawDebug
		self.update()

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

		# Simple line
		else:
			painterPath = QPainterPath(QPointF(0, 0))
			painterPath.lineTo(endPos)
			return painterPath

	'''
	Uaktualnia pozycje koncowa
	'''
	def updateEndPosition(self, endPosition):
		if self._endPosition != endPosition:
			self.prepareGeometryChange()
			self._endPosition = self.mapFromScene(endPosition)

	#
	# type property
	#

	Type = QGraphicsItem.UserType + 5

	def type(self):
		return NodeTemporaryLinkView.Type