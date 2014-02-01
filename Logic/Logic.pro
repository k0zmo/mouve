TEMPLATE = lib
TARGET = Mouve.Logic
QT -= gui
CONFIG += precompile_header

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)

# Below project includes are optional and can be commented out
include(../clw.pri)
include(../tbb.pri)

PRECOMPILED_HEADER = Precomp.h
DEFINES += LOGIC_LIB
LIBS += -lMouve.Kommon
unix: QMAKE_LFLAGS += -Wl,--rpath=.

HEADERS += \
    Node.h \
    NodeException.h \
    NodeFactory.h \
    NodeFlowData.h \
    NodeLink.h \
    NodeModule.h \
    NodePlugin.h \
    NodeProperty.h \
    NodeSystem.h \
    NodeTree.h \
    NodeTreeSerializer.h \
    NodeType.h \
    Precomp.h \
    Prerequisites.h \
    Jai/IJaiNodeModule.h \
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
    NodePlugin.cpp \
    NodeProperty.cpp \
    NodeSystem.cpp \
    NodeTree.cpp \
    NodeTreeSerializer.cpp \
    NodeType.cpp \    
    Jai/JaiCameraNodes.cpp \
    Jai/JaiNodeModule.cpp \
    Nodes/ArithmeticNodes.cpp \
    Nodes/BriskNodes.cpp \
    Nodes/ColorConversionNodes.cpp \
    Nodes/CV.cpp \
    Nodes/DrawFeaturesNodes.cpp \
    Nodes/FeatureDetectorsNodes.cpp \
    Nodes/FeaturesNodes.cpp \
    Nodes/FiltersNodes.cpp \
    Nodes/FreakNodes.cpp \
    Nodes/HistogramNodes.cpp \
    Nodes/HomographyNodes.cpp \
    Nodes/KeypointsNodes.cpp \
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

!isEmpty(HAVE_CLW) {
    win32 {
        # Create kernel DESTDIR/kernels directory if necessary
        !exists($$quote($$DESTDIR/kernels)) {
            QMAKE_POST_LINK += mkdir $$quote($$DESTDIR/kernels) $$escape_expand(\n\t)
        }

        # Copy kernel files to DESTDIR/kernels/ directory
        for(FILE, OTHER_FILES) {
            SRC_PATH = $$quote($$PWD/$${FILE})
            # Convert back slashes to forward slashes
            SRC_PATH  ~= s,/,\\,g
            DST_PATH = $$quote($$DESTDIR/kernels/)
            DST_PATH ~= s,/,\\,g
            QMAKE_POST_LINK += $$QMAKE_COPY $$SRC_PATH $$DST_PATH $$escape_expand(\n\t)
        }

        # Copy clw to DESTDIR/ directory
        CONFIG(release, debug|release): CLW_SRC_PATH = $$quote($$PWD/../external/clw/$$CLW_ARCH/clw.dll)
        else: CLW_SRC_PATH = $$quote($$PWD/../external/clw/$$CLW_ARCH/clw_d.dll)
        CLW_SRC_PATH  ~= s,/,\\,g
        CLW_DST_PATH = $$quote($$DESTDIR/)
        CLW_DST_PATH ~= s,/,\\,g
        QMAKE_POST_LINK += $$QMAKE_COPY $$CLW_SRC_PATH $$CLW_DST_PATH
    }

    unix {
        # Create kernel DESTDIR/kernels directory if necessary
        !exists($$quote($$DESTDIR/kernels)) {
            QMAKE_POST_LINK += mkdir $$quote($$DESTDIR/kernels) $$escape_expand(\n\t)
        }

        # Copy kernel files to DESTDIR/kernels/ directory
        for(FILE, OTHER_FILES) {
            SRC_PATH = $$quote($$PWD/$${FILE})
            DST_PATH = $$quote($$DESTDIR/kernels/)
            QMAKE_POST_LINK += $$QMAKE_COPY $$SRC_PATH $$DST_PATH $$escape_expand(\n\t)
        }

        # Copy clw to DESTDIR/ directory
        CONFIG(release, debug|release): CLW_SRC_PATH = $$quote($$PWD/../external/clw/$$CLW_ARCH/libclw.so)
        else: CLW_SRC_PATH = $$quote($$PWD/../external/clw/$$CLW_ARCH/libclw_d.so)
        CLW_DST_PATH = $$quote($$DESTDIR/)
        QMAKE_POST_LINK += $$QMAKE_COPY $$CLW_SRC_PATH $$CLW_DST_PATH
    }
}
