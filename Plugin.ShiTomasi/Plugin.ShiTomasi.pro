TEMPLATE  = lib
TARGET    = Plugin.ShiTomasi
QT       -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
include(../plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += ShiTomasi.cpp

HEADERS +=
