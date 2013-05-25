TEMPLATE = lib
TARGET = Mouve.Kommon
QT -= gui

include(../mouve.pri)

HEADERS += \
    HighResolutionClock.h \
    StlUtils.h \
    konfig.h \
    MacroUtils.h \  
    Singleton.h \
    Hash.h
           
SOURCES += \
    HighResolutionClock.cpp \
    Hash.cpp
