TEMPLATE = app
TARGET = Mouve.UI
QT += widgets opengl

include(../mouve.pri)
include(../clw.pri)

HEADERS += \
    Controller.h \
    GLWidget.h \
    LogView.h \
    NodeConnectorView.h \
    NodeEditorView.h \
    NodeLinkView.h \
    NodeSocketView.h \
    NodeStyle.h \
    NodeView.h \
    PreviewWidget.h \
    Prerequisites.h \
    PropertyDelegate.h \
    Property.h \
    PropertyManager.h \
    PropertyModel.h \
    PropertyWidgets.h \
    TreeWorker.h \
    GpuModuleUI.h

SOURCES += \
    Application.cpp \
    Controller.cpp \
    GLWidget.cpp \
    LogView.cpp \
    NodeConnectorView.cpp \
    NodeEditorView.cpp \
    NodeLinkView.cpp \
    NodeSocketView.cpp \
    NodeStyle.cpp \
    NodeView.cpp \
    PreviewWidget.cpp \
    Property.cpp \
    PropertyDelegate.cpp \
    PropertyManager.cpp \
    PropertyModel.cpp \
    PropertyWidgets.cpp \
    TreeWorker.cpp \
    GpuModuleUI.cpp
    
FORMS += \
    MainWindow.ui \
    PreviewWidget.ui \
    Camera.ui \
    GpuChoice.ui 
    
RESOURCES += \
    Editor.qrc

PRECOMPILED_HEADER = Precomp.h

LIBS += -lMouve.Logic -lMouve.Kommon
unix {
    QMAKE_LFLAGS += -Wl,--rpath=.
    LIBS += -lopencv_core
}

# TODO: Haven't been used for awhile now
win32 {
    LIBS += -LD:/Programowanie/SDKs/opencv/lib/x86
    # TODO: version, dir and debug|release
    LIBS += -lopencv_core243d
}
