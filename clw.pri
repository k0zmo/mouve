win32 {
    #CLW_ARCH = bin
    CLW_ARCH = bin64
    DEFINES += HAVE_OPENCL
    HAVE_CLW = true
    INCLUDEPATH += ../external/clw
    LIBS += -L$$PWD/external/clw/$$CLW_ARCH

    CONFIG(debug, debug|release) {
        LIBS += -lclw_d
    }
    CONFIG(release, debug|release) {
        LIBS += -lclw
    }
}

unix {
    #CLW_ARCH = bin
    CLW_ARCH = bin64
    DEFINES += HAVE_OPENCL
    HAVE_CLW = true
    INCLUDEPATH += ../external/clw
    LIBS += -L$$PWD/external/clw/$$CLW_ARCH

    CONFIG(debug, debug|release) {
        LIBS += -lclw_d
    }
    CONFIG(release, debug|release) {
        LIBS += -lclw
    }
}
