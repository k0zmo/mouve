TEMPLATE  = lib
TARGET    = Plugin.Kuwahara
QT       -= gui

include(../Include/mouve.pri)
include(../Include/boost.pri)
include(../Include/opencv.pri)
include(../Include/plugin.pri)
# Below project includes are optional and can be commented out
include(../Include/clw.pri)

LIBS += -lMouve.Logic -lMouve.Kommon

SOURCES += \
    cvu.cpp \
    main.cpp \
    GpuKuwaharaFilterNode.cpp \
    KuwaharaFilterNode.cpp

HEADERS += \
    cvu.h \

OTHER_FILES += \
    kernels/kuwahara.cl

!isEmpty(HAVE_CLW) {
    win32 {
        # Copy kernel files to DESTDIR/kernels/ directory
        for(FILE, OTHER_FILES) {
            SRC_PATH = $$quote($$PWD/$${FILE})
            # Convert back slashes to forward slashes
            SRC_PATH  ~= s,/,\\,g
            DST_PATH = $$quote($$DESTDIR/../kernels/)
            DST_PATH ~= s,/,\\,g
            QMAKE_POST_LINK += $$QMAKE_COPY $$SRC_PATH $$DST_PATH $$escape_expand(\n\t)
        }
    }

    unix {
        # Copy kernel files to DESTDIR/kernels/ directory
        for(FILE, OTHER_FILES) {
            SRC_PATH = $$quote($$PWD/$${FILE})
            DST_PATH = $$quote($$DESTDIR/../kernels/)
            QMAKE_POST_LINK += $$QMAKE_COPY $$SRC_PATH $$DST_PATH $$escape_expand(\n\t)
        }
    }
}
