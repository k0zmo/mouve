from PySide.QtCore import *
from PySide.QtGui import *

from NodeLinkView import *
from NodeView import *

class NodeManager(QObject):
	def __init__(self, parent=None):
		QObject.__init__(self, parent)

		self.linkSelected = None

	def eventFilter(self, obj, event):
		# if type(obj) is NodeLinkView:
		# 	if event.type() == QEvent.GraphicsSceneMousePress:
		# 		return True # This is must so mouseRelease event is generated

		# 	elif event.type() == QEvent.GraphicsSceneMouseRelease:
		# 		if self.linkSelected is not obj:
		# 			obj.pen.setStyle(Qt.DashLine)
		# 			obj.update()
		# 			if self.linkSelected:
		# 				self.linkSelected.pen.setStyle(Qt.SolidLine)
		# 				self.linkSelected.update()
		# 			self.linkSelected = obj
		# 		return True

		# 	elif event.type() == QEvent.KeyPress:
		# 		if event.key() == Qt.Key_Delete:
		# 			print 'delete'
		# 		return True

		if type(obj) == NodeView:
			#print event.type()
			pass

		return QObject.eventFilter(self, obj, event)
		
		# # if event.type() == QEvent.GraphicsSceneContextMenu:
		# # 	print 'context', event.scenePos()
		# # 	return True
		# if event.type() == QEvent.GraphicsSceneMousePress:
		# 	if event.button() == Qt.RightButton:
		# 		menu = QMenu()
		# 		menu.addAction('qwe')
		# 		menu.addAction('asd')
		# 		menu.exec_(event.globalPos())
		# 	return True
		# else:
		# 	# standard event processing
		# 	return QObject.eventFilter(self, obj, event)