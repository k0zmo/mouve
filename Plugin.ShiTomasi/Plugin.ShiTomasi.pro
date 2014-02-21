TEMPLATE  = lib
TARGET    = Plugin.ShiTomasi
QT       -= gui

include(../Include/mouve.pri)
include(../Include/boost.pri)
include(../Include/opencv.pri)
include(../Include/plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += ShiTomasi.cpp

HEADERS +=
