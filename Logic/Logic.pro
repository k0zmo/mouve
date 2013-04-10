TEMPLATE = lib
TARGET = Mouve.Logic
QT -= gui

include(../mouve.pri)
include(../clw.pri)

HEADERS += \
	Node.h \
    NodeException.h \
	NodeFactory.h \
    NodeFlowData.h \
	NodeLink.h \
    NodeModule.h \
	NodeSystem.h \
	NodeTree.h \
	NodeType.h \
    Prerequisites.h \
    Jai/JaiException.h \
    Jai/JaiNodeModule.h \
    Nodes/CV.h \
    OpenCL/DeviceArray.h \
    OpenCL/GpuException.h \
    OpenCL/GpuKernelLibrary.h \
    OpenCL/GpuNode.h \
    OpenCL/GpuNodeModule.h
		   
SOURCES += \
	Node.cpp \
	NodeFactory.cpp \
    NodeFlowData.cpp \
	NodeLink.cpp \
    NodeModule.cpp \
	NodeSystem.cpp \
	NodeTree.cpp \
    NodeType.cpp \
    Jai/JaiCameraNodes.cpp \
    Jai/JaiNodeModule.cpp \
    Nodes/BasicNodes.cpp \
    Nodes/BriskNodes.cpp \
    Nodes/ColorConversionNodes.cpp \
    Nodes/CV.cpp \
    Nodes/DrawFeaturesNodes.cpp \
    Nodes/HomographyNodes.cpp \
    Nodes/HoughNodes.cpp \
    Nodes/MatcherNodes.cpp \
    Nodes/SurfNodes.cpp \
    OpenCL/DeviceArray.cpp \
    OpenCL/GpuBasicNodes.cpp \
    OpenCL/GpuHoughLinesNode.cpp \
    OpenCL/GpuKernelLibrary.cpp \
    OpenCL/GpuMixtureOfGaussians.cpp \
    OpenCL/GpuMorphologyNode.cpp \
    OpenCL/GpuNodeModule.cpp
	
PRECOMPILED_HEADER = Precomp.h
DEFINES += LOGIC_LIB
LIBS += -lMouve.Kommon
unix {
    QMAKE_LFLAGS += -Wl,--rpath=.
	LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video
    LIBS += -lopencv_flann -lopencv_calib3d -lopencv_features2d -lopencv_nonfree
}

# TODO: Haven't been used for awhile now
win32 {
    LIBS += -LD:/Programowanie/SDKs/opencv/lib/x86
    # TODO: version, dir and debug|release
	LIBS += -lopencv_core243d -lopencv_imgproc243d -lopencv_highgui243d -lopencv_video243d
    LIBS += -lopencv_flann243d -lopencv_calib3d243d -lopencv_features2d243d -lopencv_nonfree243d

    INCLUDEPATH += D:/Programowanie/SDKs/boost
}
