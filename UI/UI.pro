TEMPLATE = app
TARGET = Mouve.UI
QT += widgets opengl

include(../mouve.pri)

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
	TreeWorker.h

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
	TreeWorker.cpp
	
FORMS += \
    MainWindow.ui \
    PreviewWidget.ui
	
RESOURCES += \
	Editor.qrc

PRECOMPILED_HEADER = Precomp.h

LIBS += -lMouve.Logic

unix {
    LIBS += -lopencv_core
}
win32 {
    LIBS += -LD:/Programowanie/SDKs/opencv/lib/x86
    # TODO: version, dir and debug|release
    LIBS += -lopencv_core243d
}
