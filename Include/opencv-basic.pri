win32 {
    OPENCV_DIR = D:/Programowanie/SDKs/opencv/build
    OPENCV_VERSION = 248
    OPENCV_ARCH = x86
    #OPENCV_ARCH = x64

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -L$$OPENCV_DIR/$$OPENCV_ARCH/vc12/lib
    LIBS += -lopencv_core$$OPENCV_VERSION
}

unix {
    OPENCV_DIR = /usr/include

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -lopencv_core
}
