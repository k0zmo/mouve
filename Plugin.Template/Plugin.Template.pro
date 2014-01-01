TEMPLATE  = lib
TARGET    = Plugin.Template
QT       -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
include(../plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += main.cpp

HEADERS +=
