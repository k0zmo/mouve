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
    NodeEditor/NodeConnectorView.h \
    NodeEditor/NodeEditorView.h \
    NodeEditor/NodeLinkView.h \
    NodeEditor/NodeSocketView.h \
    NodeEditor/NodeStyle.h \
    NodeEditor/NodeToolTip.h \
    NodeEditor/NodeView.h \
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
    NodeEditor/NodeConnectorView.cpp \
    NodeEditor/NodeEditorView.cpp \
    NodeEditor/NodeLinkView.cpp \
    NodeEditor/NodeSocketView.cpp \
    NodeEditor/NodeStyle.cpp \
    NodeEditor/NodeToolTip.cpp \
    NodeEditor/NodeView.cpp \
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
