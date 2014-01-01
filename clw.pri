win32 {
    CLW_ARCH = bin64
    DEFINES += HAVE_OPENCL_1_1
    #DEFINES += HAVE_OPENCL_1_2
    DEFINES += HAVE_OPENCL
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
    CLW_ARCH = bin64
    DEFINES += HAVE_OPENCL_1_1
    #DEFINES += HAVE_OPENCL_1_2
    DEFINES += HAVE_OPENCL
    INCLUDEPATH += ../external/clw
    LIBS += -L$$PWD/external/clw/$$CLW_ARCH

    CONFIG(debug, debug|release) {
        LIBS += -lclw_d
    }
    CONFIG(release, debug|release) {
        LIBS += -lclw
    }
}
