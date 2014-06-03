win32 {
    TBB_DIR = D:/Programowanie/SDKs/tbb41
    TBB_ARCH = ia32
    #TBB_ARCH = intel64

    INCLUDEPATH += $$TBB_DIR/include
    DEFINES += NOMINMAX HAVE_TBB

    LIBS += -L$$TBB_DIR/lib/$$TBB_ARCH/vc11

    CONFIG(debug, debug|release) {
        LIBS += -ltbb_debug
    }
    CONFIG(release, debug|release) {
        LIBS += -ltbb
    }
}

unix {
    TBB_DIR = /usr/include

    INCLUDEPATH += $$TBB_DIR/include
    DEFINES += HAVE_TBB
    LIBS += -ltbb
}
