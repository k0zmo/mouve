unix {
	DEFINES += HAVE_OPENCL_1_1
	DEFINES += HAVE_OPENCL
    INCLUDEPATH += ../external/clw
    
	CONFIG(debug, debug|release) {
		LIBS += -lclw_d
	}
	CONFIG(release, debug|release) {
		LIBS += -lclw
	}
}


