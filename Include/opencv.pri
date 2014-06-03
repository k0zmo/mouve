win32 {
    OPENCV_DIR = D:/Programowanie/SDKs/opencv/build
    OPENCV_VERSION = 248
    OPENCV_ARCH = x86
    #OPENCV_ARCH = x64

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -L$$OPENCV_DIR/$$OPENCV_ARCH/vc12/lib
    LIBS += -lopencv_core$$OPENCV_VERSION \
            -lopencv_imgproc$$OPENCV_VERSION \
            -lopencv_highgui$$OPENCV_VERSION \
            -lopencv_video$$OPENCV_VERSION \
            -lopencv_features2d$$OPENCV_VERSION \
            -lopencv_nonfree$$OPENCV_VERSION \
            -lopencv_flann$$OPENCV_VERSION \
            -lopencv_calib3d$$OPENCV_VERSION
}

unix {
    OPENCV_DIR = /usr/include

    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -lopencv_core \
            -lopencv_imgproc\
            -lopencv_highgui \
            -lopencv_video \
            -lopencv_features2d \
            -lopencv_nonfree \
            -lopencv_flann \
            -lopencv_calib3d
}
