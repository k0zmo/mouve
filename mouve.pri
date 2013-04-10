CONFIG(release, debug|release): {
    OBJECTS_DIR=$$OUT_PWD/../obj/$$TARGET/release
    contains(DEFINES, QT_MOC) {
        DESTDIR=$$OBJECTS_DIR
    } else {
        DESTDIR=$$OUT_PWD/../bin/release
    }
} else {
    OBJECTS_DIR=$$OUT_PWD/../obj/$$TARGET/debug
    contains(DEFINES, QT_MOC) {
        DESTDIR=$$OBJECTS_DIR
    } else {
        DESTDIR=$$OUT_PWD/../bin/debug
    }
}

CONFIG += c++11
INCLUDEPATH += ../
LIBS += -L$$DESTDIR

win32 {
    INCLUDEPATH += D:/Programowanie/SDKs/opencv/include
}
