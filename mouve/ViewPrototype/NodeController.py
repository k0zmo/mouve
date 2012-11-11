from PySide.QtCore import *
from PySide.QtGui import *
from NodeScene import *

controller = None
DEBUG_LINKS = False

class NodeController(QObject):

	# These ones goes to model
	_registeredNodeTypes = ['Image from disk', 'Gaussian blur', \
				'Preview window', 'Grayscale', 'Mixture of Gaussians']
	_nodeId = 0

	def __init__(self, parent=None):
		QObject.__init__(self, parent)

		self._nodeViews = {}
		self._linkViews = []

		self._scene = NodeScene(self)
		# !TODO: Temporary
		self._scene.setSceneRect(-200,-200,1000,600)

		# !TODO: Move to some initialize method
		self._view = NodeEditorView()
		self._view.setScene(self._scene)
		self._view.setGeometry(100, 50, 1200, 800)
		self._view.contextMenu.connect(self.contextMenu)
		self._view.show()

		self._addNodesActions = []
		for item in enumerate(self._registeredNodeTypes):
			a = QAction(item[1], self)
			a.setData(item[0])
			self._addNodesActions.append(a)

	'''
	Zwraca widok wezla o podanym kluczu ID
	'''
	def nodeView(self, key):
		return self._nodeViews[key]

	'''
	Dodaje nowy widok wezla
	'''
	def addNodeView(self, key, title):
		node = NodeView(title)
		node.setData(NodeDataIndex.NodeKey, key)
		#node.installEventFilter(self.nodeManager)
		self._nodeViews[key] = node
		self._scene.addItem(node)
		return node

	''' 
	Usuwa widok wezla o podanym kluczu ID
	'''
	def deleteNodeView(self, key):
		print 'deleting node with key:', key
		# if key in self._nodeViews:
		# 	print 'deleting node with key: {0}'.format(key)
		# 	nodeView = self._nodeViews.pop(key)
		# 	# !TODO: Nie pamietam juz czy logika sie tym zajmie czy nie
		# 	#         - chodzi o usuniecie wiszacych polaczen
		# 	lvs = filter(lambda lv: lv.connecting(nodeView), self._linkViews)
		# 	for link in lvs:
		# 		link.dtor()
		# 		self._linkViews.remove(link)

		# 	self._scene.removeItem(nodeView)
		# 	return True
		# else:
		# 	print 'node with key: {0} not found'.format(key)
		# 	return False

	'''
	Zwraca liste polaczen wchodzacych do danego wezla
	'''
	def inputLinkViews(self, nodeKey):
		if nodeKey in self._nodeViews:
			nv = self._nodeViews[nodeKey]
			
			return filter(lambda lv: lv.inputConnecting(nv), self._linkViews)
		return None

	'''
	Zwraca liste polaczen wychodzacych z danego wezla
	'''
	def outputLinkViews(self, nodeKey):
		if nodeKey in self._nodeViews:
			nv = self._nodeViews[nodeKey]
			
			return filter(lambda lv: lv.outputConnecting(nv), self._linkViews)
		return None

	'''
	Tworzy nowe (wizualne) polaczenie miedzy danymi gniazdami
	'''
	def linkNodeViews(self, fromSocketView, toSocketView):
		# TODO!: To bedzie w logice
		# Check if we aren't trying to create already existing connection
		for linkView in self._linkViews:
			if linkView.connects(fromSocketView, toSocketView):
				QMessageBox.critical(None, 'NodeView', 'Connection already exists')
				return

		link = NodeLinkView(fromSocketView, toSocketView, None)
		link.setDrawDebug(DEBUG_LINKS)
		fromSocketView.addLink(link)
		toSocketView.addLink(link)

		self._linkViews.append(link)
		#link.installEventFilter(self.nodeManager)
		self._scene.addItem(link)

	'''
	Usuwa polaczenie (wizualne) miedzy danymi gniazdami
	'''
	def unlinkNodeViews2(self, fromSocketView, toSocketView):
		for linkView in self._linkViews:
			if linkView.connects(fromSocketView, toSocketView):
				self._scene.removeItem(linkView)
				self._linkViews.remove(linkView)
				return True
		return False

	def unlinkNodeViews(self, linkView):
		pass

	# Normalnie, za to odpowiedzialna jest logika
	def _addNode(self, nodeTypeId, scenePos):
		if nodeTypeId < len(self._registeredNodeTypes):
			title = self._registeredNodeTypes[nodeTypeId]
			nodeId = self._nodeId
			node = self.addNodeView(nodeId, title)
			node.setPos(scenePos)
			if title == 'Image from disk':
				node.addSocketView(0, 'Output', True)
			elif title == 'Gaussian blur':
				node.addSocketView(0, 'Input')
				node.addSocketView(1, 'Sigma')
				node.addSocketView(0, 'Output', True)
			elif title == 'Preview window':
				node.addSocketView(0, 'Input')
			elif title == 'Grayscale':
				node.addSocketView(0, 'Input')
				node.addSocketView(0, 'Output', True)
			elif title == 'Mixture of Gaussians':
				node.addSocketView(0, 'Frame')
				node.addSocketView(0, 'Foreground mask', True)
			self._nodeId += 1

	'''
	Reakcja na opuszczenie nowego polaczenia
	'''
	@Slot(NodeSocketView, NodeSocketView)
	def draggingLinkDropped(self, fromSocketView, toSocketView):
		self.linkNodeViews(fromSocketView, toSocketView)

	@Slot(NodeSocketView)
	def draggingLinkStarted(self, fromSocketView):
		self._scene.dragging = True
		#print 'dragging started from', fromSocketView

	@Slot(NodeSocketView)
	def draggingLinkStopped(self, fromSocketView):
		self._scene.dragging = False
		#print 'dragging stopped'

	'''
	Reakcja na probe pokazania menu kontekstowego pochodzacego z widoku sceny
	'''
	@Slot(QPoint, QPointF)
	def contextMenu(self, globalPos, scenePos):
		items = self._scene.items(scenePos, Qt.ContainsItemShape, Qt.AscendingOrder)
		# If we clicked onto empty space
		if not items:
			menu = QMenu()
			menuAddNode = menu.addMenu('Add node')
			for a in self._addNodesActions:
		 		menuAddNode.addAction(a)
		 	ret = menu.exec_(globalPos)
		 	if ret is not None:
		 		self._addNode(ret.data(), scenePos)
		else:
			for item in items:
				if item.type() == NodeView.Type:
					menu = QMenu()
					action = QAction('Delete node', None)
					action.setData(10)
					menu.addAction(action)
					ret = menu.exec_(globalPos)
					if ret is not None and isinstance(ret, QAction):
						#print item.nodeKey()
						self.deleteNodeView(item.nodeKey())
					break

	def _init_sample_scene(self):
		self._addNode(self._registeredNodeTypes.index('Image from disk'), QPoint(-250,0))
		self._addNode(self._registeredNodeTypes.index('Grayscale'), QPoint(-40,50))
		self._addNode(self._registeredNodeTypes.index('Gaussian blur'), QPoint(150,100))
		self._addNode(self._registeredNodeTypes.index('Mixture of Gaussians'), QPoint(350,150))
		self._addNode(self._registeredNodeTypes.index('Preview window'), QPoint(600,100))

		#self.linkNodeViews(sv1,sv2)
		#self.linkNodeViews(sv1,sv5)
		#self.linkNodeViews(sv3,s4v)

		#self.deleteNodeView(1)
		#self.unlinkNodeViews2(sv1, sv2)

