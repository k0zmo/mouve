TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
	Common \
	Logic \
	UI

UI.depends = Logic
