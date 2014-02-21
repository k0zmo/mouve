TEMPLATE = app
TARGET = Mouve.Console
QT -= gui

include(../Include/mouve.pri)
include(../Include/boost.pri)
include(../Include/opencv.pri)
# Below project includes are optional and can be commented out
include(../Include/clw.pri)

LIBS += -lMouve.Logic -lMouve.Kommon
unix: QMAKE_LFLAGS += -Wl,--rpath=.

SOURCES += \
    main.cpp
