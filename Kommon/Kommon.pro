TEMPLATE = lib
TARGET = Mouve.Kommon
QT -= gui
CONFIG += staticlib

include(../mouve.pri)

HEADERS += \
    HighResolutionClock.h \
    StlUtils.h \
    konfig.h \
    MacroUtils.h \
    Singleton.h \
    Hash.h \
    SharedLibrary.h

SOURCES += \
    HighResolutionClock.cpp \
    Hash.cpp \
    SharedLibrary.cpp
