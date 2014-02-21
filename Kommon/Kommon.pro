TEMPLATE = lib
TARGET = Mouve.Kommon
QT -= gui
CONFIG += staticlib

include(../Include/mouve.pri)

HEADERS += \
    HighResolutionClock.h \
    StlUtils.h \
    konfig.h \
    MacroUtils.h \
    Singleton.h \
    Hash.h \
    SharedLibrary.h \
    EnumFlags.h \
    TypeTraits.h \
    StringUtils.h

SOURCES += \
    HighResolutionClock.cpp \
    Hash.cpp \
    SharedLibrary.cpp \
    StringUtils.cpp
