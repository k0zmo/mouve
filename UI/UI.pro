TEMPLATE = app
TARGET = Mouve.UI
QT += widgets opengl
CONFIG += precompile_header

include(../mouve.pri)
include(../clw.pri)

HEADERS += \
    Controller.h \
    GLWidget.h \
    GpuModuleUI.h \
    LogView.h \
    NodeConnectorView.h \
    NodeEditorView.h \
    NodeLinkView.h \
    NodeSocketView.h \
    NodeStyle.h \
    NodeToolTip.h \
    NodeView.h \
    PreviewWidget.h \
    Prerequisites.h \
    PropertyDelegate.h \
    Property.h \
    PropertyManager.h \
    PropertyModel.h \
    PropertyWidgets.h \
    TreeWorker.h \
    Precomp.h

SOURCES += \
    Application.cpp \
    Controller.cpp \
    GLWidget.cpp \
    GpuModuleUI.cpp \
    LogView.cpp \
    NodeConnectorView.cpp \
    NodeEditorView.cpp \
    NodeLinkView.cpp \
    NodeSocketView.cpp \
    NodeStyle.cpp \
    NodeToolTip.cpp \
    NodeView.cpp \
    PreviewWidget.cpp \
    Property.cpp \
    PropertyDelegate.cpp \
    PropertyManager.cpp \
    PropertyModel.cpp \
    PropertyWidgets.cpp \
    TreeWorker.cpp \

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
