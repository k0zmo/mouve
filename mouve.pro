TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
	Kommon \
	Logic \
	UI

UI.depends = Logic
