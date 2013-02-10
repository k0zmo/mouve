from PySide.QtCore import *
from PySide.QtGui import *
import os

FontName = 'Segoe UI' if os.name == 'nt' else 'DejaVu Sans'

class NodeStyle:
	# Scene NodeStyle
	SceneBackground = QColor(64, 64, 64)

	# Link NodeStyle
	LinkPen = QPen(QColor(222, 222, 222), 1.5, Qt.SolidLine)
	#LinkPen = QPen(QColor(16, 16, 16), 1.5, Qt.SolidLine)

	# Node NodeStyle
	NodeTitleFont = QFont(FontName, 12, QFont.Bold)
	NodeTitleFontBrush = QBrush(QColor(180, 180, 180))
	NodeTitleBrush = QBrush(QColor(62, 62, 62))
	NodeBrush = QBrush(QColor(37, 37, 37))
	NodeBorderPen = QPen(QColor(88, 88, 88), 1.0, Qt.SolidLine)
	NodeBorderSelectedPen = QPen(QColor(88, 88, 88), 1.5, Qt.SolidLine)
	NodeRoundArc = 15
	NodeTitleSize = 5.0
	NodeTitleHorizontalMargin = 30.0
	NodeSocketHorizontalMargin = 5.0
	NodeSocketVerticalMargin = 8.0
	NodeSocketsMargin = 20.0

	# Socket NodeStyle
	SocketGradientStart = QColor(93, 93, 93)
	SocketGradientStop = QColor(57, 57, 57)
	SocketPen = QPen(QColor(180, 180, 180), 1.0, Qt.SolidLine)
	SocketSize = QRect(0, 0, 15, 15)

	SocketTitleBrush = QBrush(QColor(180, 180, 180))
	SocketTitleInactiveBrush = QBrush(QColor(119, 119, 119))

	SocketFont = QFont(FontName, 11)

	ZValueNode = 0
	ZValueNodeHovered = 2000
	ZValueLink = 3000
	ZValueTemporaryLink = 4000