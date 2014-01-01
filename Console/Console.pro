TEMPLATE = app
TARGET = Mouve.Console
QT -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
# Below project includes are optional and can be commented out
include(../clw.pri)

LIBS += -lMouve.Logic -lMouve.Kommon
unix: QMAKE_LFLAGS += -Wl,--rpath=.

SOURCES += \
    main.cpp
