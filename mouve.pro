TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
	Kommon \
	Logic \
	UI \
    ShiTomasi.Plugin \
    BRISK.Plugin

Logic.depends = Kommon
UI.depends = Logic
ShiTomasi.Plugin.depends = Logic
BRISK.Plugin.depends = Logic