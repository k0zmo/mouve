TEMPLATE = lib
TARGET = Mouve.Kommon
QT -= gui
CONFIG += staticlib

include(../Include/mouve.pri)

HEADERS += \
    HighResolutionClock.h \
    konfig.h \
    MacroUtils.h \
    Singleton.h \
    Hash.h \
    SharedLibrary.h \
    EnumFlags.h \
    TypeTraits.h \
    StringUtils.h \
    ModulePath.h \
    NestedException.h \
    Utils.h \
    json11.h 

SOURCES += \
    HighResolutionClock.cpp \
    Hash.cpp \
    SharedLibrary.cpp \
    StringUtils.cpp \
    ModulePath.cpp \
    json11.cpp
