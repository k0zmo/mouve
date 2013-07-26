QT       -= gui
TARGET    = BRISK.Plugin
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

SOURCES += brisk.cpp \
    BriskNode.cpp \
    agast/agast5_8.cc \
    agast/agast5_8_nms.cc \
    agast/agast7_12d.cc \
    agast/agast7_12d_nms.cc \
    agast/agast7_12s.cc \
    agast/agast7_12s_nms.cc \
    agast/AstDetector.cc \
    agast/nonMaximumSuppression.cc \
    agast/oast9_16.cc \
    agast/oast9_16_nms.cc \

HEADERS += brisk.h \
    agast\agast5_8.h \
    agast\agast7_12d.h \
    agast\agast7_12s.h \
    agast\AstDetector.h \
    agast\cvWrapper.h \
    agast\oast9_16.h

INCLUDEPATH += ../Logic

LIBS += -lMouve.Logic -lMouve.Kommon
unix {
    LIBS += -lopencv_core -lopencv_imgproc -lopencv_features2d
    QMAKE_CXXFLAGS += -mssse3
}

