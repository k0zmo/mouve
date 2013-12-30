QT       -= gui
TARGET    = Plugin.Kuwahara
TEMPLATE  = lib

include(../mouve.pri)
include(../clw.pri)

CONFIG(release, debug|release): {
    OBJECTS_DIR=$$OUT_PWD/../obj/$$TARGET/release
    contains(DEFINES, QT_MOC) {
        DESTDIR=$$OBJECTS_DIR
    } else {
        DESTDIR=$$OUT_PWD/../bin/release/plugins
    }
} else {
    OBJECTS_DIR=$$OUT_PWD/../obj/$$TARGET/debug
    contains(DEFINES, QT_MOC) {
        DESTDIR=$$OBJECTS_DIR
    } else {
        DESTDIR=$$OUT_PWD/../bin/debug/plugins
    }
}

SOURCES += main.cpp
HEADERS += 
INCLUDEPATH += ../Logic

LIBS += -lMouve.Logic -lMouve.Kommon
unix {
    LIBS += -lopencv_core
    QMAKE_CXXFLAGS += -mssse3
}

