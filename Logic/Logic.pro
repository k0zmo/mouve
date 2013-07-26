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
    NodePlugin.h \
    NodeTreeSerializer.h \
    Prerequisites.h \
    Jai/JaiException.h \
    Jai/JaiNodeModule.h \
    Nodes/CV.h \
    Nodes/ksurf.h \
    OpenCL/DeviceArray.h \
    OpenCL/GpuException.h \
    OpenCL/GpuKernelLibrary.h \
    OpenCL/GpuNode.h \
    OpenCL/GpuNodeModule.h
    OpenCL/IGpuNodeModule.h
           
SOURCES += \
    Node.cpp \
    NodeFactory.cpp \
    NodeFlowData.cpp \
    NodeLink.cpp \
    NodeSystem.cpp \
    NodeTree.cpp \
    NodeType.cpp \
    NodePlugin.cpp \
    NodeTreeSerializer.cpp \
    Jai/JaiCameraNodes.cpp \
    Jai/JaiNodeModule.cpp \
    Nodes/ArithmeticNodes.cpp \
    Nodes/BriskNodes.cpp \
    Nodes/ColorConversionNodes.cpp \
    Nodes/CV.cpp \
    Nodes/DrawFeaturesNodes.cpp \
    Nodes/FeatureDetectorsNodes.cpp \
    Nodes/FiltersNodes.cpp \
    Nodes/FreakNodes.cpp \
    Nodes/HistogramNodes.cpp \
    Nodes/HomographyNodes.cpp \
    Nodes/ksurf.cpp \
    Nodes/kSurfNodes.cpp \
    Nodes/MatcherNodes.cpp \
    Nodes/MorphologyNodes.cpp \
    Nodes/MosaicingNodes.cpp \
    Nodes/OrbNodes.cpp \
    Nodes/SegmentationNodes.cpp \
    Nodes/SiftNodes.cpp \
    Nodes/SourceNodes.cpp \
    Nodes/SurfNodes.cpp \
    Nodes/TransformationNodes.cpp \
    Nodes/VideoSegmentationNodes.cpp \
    OpenCL/DeviceArray.cpp \
    OpenCL/GpuBasicNodes.cpp \
    OpenCL/GpuBruteForceMacherNode.cpp \
    OpenCL/GpuColorConversionNodes.cpp \
    OpenCL/GpuHoughLinesNode.cpp \
    OpenCL/GpuKernelLibrary.cpp \
    OpenCL/GpuMixtureOfGaussians.cpp \
    OpenCL/GpuMorphologyOperatorNode.cpp \
    OpenCL/GpuNodeModule.cpp \
    OpenCL/GpuSurfNode.cpp

OTHER_FILES += \
    OpenCL/kernels/bfmatcher.cl \
    OpenCL/kernels/bfmatcher_macros.cl \
    OpenCL/kernels/bayer.cl \
    OpenCL/kernels/fill.cl \
    OpenCL/kernels/hough.cl \
    OpenCL/kernels/integral.cl \
    OpenCL/kernels/mog.cl \
    OpenCL/kernels/morphOp.cl \
    OpenCL/kernels/surf.cl

PRECOMPILED_HEADER = Precomp.h
DEFINES += LOGIC_LIB
LIBS += -lMouve.Kommon
INCLUDEPATH += .
unix {
    QMAKE_LFLAGS += -Wl,--rpath=.

    !exists($$quote($$DESTDIR/kernels)) {
        QMAKE_POST_LINK += mkdir $$quote($$DESTDIR/kernels) $$escape_expand(\n\t)
    }

    # Copy kernel files to DESTDIR/kernels/ directory
    for(FILE, OTHER_FILES) {
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$PWD/$${FILE}) $$quote($$DESTDIR/kernels/) $$escape_expand(\n\t)
    }

    # Copy clw to DESTDIR/ directory
    CONFIG(release, debug|release): {
        QMAKE_PRE_LINK += $$QMAKE_COPY $$quote($$PWD/../external/clw/bin64/libclw.so) $$quote($$DESTDIR/)
    } else {
        QMAKE_PRE_LINK += $$QMAKE_COPY $$quote($$PWD/../external/clw/bin64/libclw_d.so) $$quote($$DESTDIR/)
    }
    # OpenCV
    LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video -lopencv_flann -lopencv_calib3d -lopencv_features2d -lopencv_nonfree

    # Intel TBB
    DEFINES += HAVE_TBB
    LIBS += -ltbb

    # Boost should be in common path (like /usr/include
}
