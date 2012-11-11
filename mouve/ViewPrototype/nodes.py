#!/usr/bin/python2

import sys

from PySide.QtCore import *
from PySide.QtGui import *
import NodeController

def main():
	app = QApplication(sys.argv)
	NodeController.controller = NodeController.NodeController()
	NodeController.controller._init_sample_scene()
	sys.exit(app.exec_())

if __name__ == '__main__':
	main()