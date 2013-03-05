TEMPLATE = lib
TARGET = Mouve.Logic
QT -= gui

include(../mouve.pri)

HEADERS += \
	CV.h \
	Node.h \
	NodeFactory.h \
	NodeLink.h \
	NodeSystem.h \
	NodeTree.h \
	NodeType.h \
	Prerequisites.h
		   
SOURCES += \
	BuiltInNodeTypes.cpp \
	CV.cpp \
	Node.cpp \
	NodeFactory.cpp \
	NodeLink.cpp \
	NodeSystem.cpp \
	NodeTree.cpp \
	NodeType.cpp
	
PRECOMPILED_HEADER = Precomp.h

DEFINES += MOUVE_LOGIC_LIB

unix {
	LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video
}
win32 {
	LIBS += -LD:/Programowanie/Libs/opencv-243/lib/x86
	LIBS += -lopencv_core243d -lopencv_imgproc243d -lopencv_highgui243d -lopencv_video243d
}
