TEMPLATE  = lib
TARGET    = Plugin.Template
QT       -= gui

include(../Include/mouve.pri)
include(../Include/boost.pri)
include(../Include/opencv.pri)
include(../Include/plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += \
    main.cpp \
    TemplateNode.cpp

HEADERS +=
