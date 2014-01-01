TEMPLATE  = lib
TARGET    = Plugin.BRISK
QT       -= gui

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
include(../plugin.pri)

LIBS += -lMouve.Logic -lMouve.Kommon
unix: QMAKE_CXXFLAGS += -mssse3

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
