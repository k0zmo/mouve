TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
	Kommon \
	Logic \
	UI \
    ShiTomasi.Plugin

UI.depends = Logic
