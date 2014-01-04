win32 {
    OPENCV_DIR = D:/Programowanie/SDKs/opencv
    OPENCV_VERSION = 245
    #OPENCV_ARCH = x86
    OPENCV_ARCH = x64

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -L$$OPENCV_DIR/lib/$$OPENCV_ARCH
    LIBS += -lopencv_core$$OPENCV_VERSION
}

unix {
    OPENCV_DIR = /usr/include

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -lopencv_core
}
