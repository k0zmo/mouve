TEMPLATE = lib
TARGET = Mouve.Logic
QT -= gui

include(../mouve.pri)

HEADERS += \
	CV.h \
	Node.h \
    NodeException.h \
	NodeFactory.h \
    NodeFlowData.h \
	NodeLink.h \
	NodeSystem.h \
	NodeTree.h \
	NodeType.h \
	Prerequisites.h
		   
SOURCES += \
    BuiltInNodeBasics.cpp \
    BuiltInNodeFeatures.cpp \
	CV.cpp \
	Node.cpp \
	NodeFactory.cpp \
    NodeFlowData.cpp \
	NodeLink.cpp \
	NodeSystem.cpp \
	NodeTree.cpp \
	NodeType.cpp
	
PRECOMPILED_HEADER = Precomp.h

DEFINES += MOUVE_LOGIC_LIB

unix {
	LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video
    LIBS += -lopencv_flann -lopencv_calib3d -lopencv_features2d -lopencv_nonfree
}
win32 {
    LIBS += -LD:/Programowanie/SDKs/opencv/lib/x86
    # TODO: version, dir and debug|release
	LIBS += -lopencv_core243d -lopencv_imgproc243d -lopencv_highgui243d -lopencv_video243d
    LIBS += -lopencv_flann243d -lopencv_calib3d243d -lopencv_features2d243d -lopencv_nonfree243d

    INCLUDEPATH += D:/Programowanie/SDKs/boost
}
