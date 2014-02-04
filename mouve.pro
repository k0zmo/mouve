TEMPLATE = subdirs
SUBDIRS = Kommon Logic Console UI \
    Plugin.BRISK \
    Plugin.ShiTomasi \
    Plugin.Template \
    Plugin.Kuwahara
CONFIG += ordered

Logic.depends = Kommon
Console.depends = Logic
UI.depends = Logic
Plugin.BRISK.depends = Logic
Plugin.ShiTomasi.depends = Logic
Plugin.Template.depends = Logic
Plugin.Kuwahara.depends = Logic
