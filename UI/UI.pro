TEMPLATE = app
TARGET = Mouve.UI
QT += widgets opengl
CONFIG += precompile_header

include(../mouve.pri)
include(../boost.pri)
include(../opencv.pri)
# Below project includes are optional and can be commented out
include(../clw.pri)

PRECOMPILED_HEADER = Precomp.h
INCLUDEPATH += .
LIBS += -lMouve.Logic -lMouve.Kommon
unix: QMAKE_LFLAGS += -Wl,--rpath=.
win32: RC_FILE = Mouve.UI.rc

HEADERS += \
    Controller.h \
    GLWidget.h \
    GpuChoiceDialog.h \
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
    GpuChoiceDialog.cpp \
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
