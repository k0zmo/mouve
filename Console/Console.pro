TEMPLATE = app
TARGET = Mouve.Console
QT -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += \
    main.cpp
