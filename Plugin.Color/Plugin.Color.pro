TEMPLATE  = lib
TARGET    = Plugin.Color
QT       -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
include(../plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += \
    main.cpp \
    ImageFromFileRgbNodeType.cpp \
    VideoFromFileRgbNodeType.cpp