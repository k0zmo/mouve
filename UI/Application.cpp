#include <QApplication>
#include <QGLFormat>
#include "Controller.h"

#if defined(QT_DEBUG) && defined(HAVE_VLD)
#  include <vld.h>
#endif

#if defined(DEBUGGING_CONSOLE)
#  include <Windows.h>
#endif

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	//QApplication::setFont(QFont());

#if defined(DEBUGGING_CONSOLE)
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
#endif

	if (!QGLFormat::hasOpenGL())
	{
		qWarning("This system has no OpenGL support. Exiting.");
		return -1;
	}

	Controller controller;
	controller.show();
	return a.exec();
}
