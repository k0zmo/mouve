#include <QApplication>
#include "Controller.h"

#if _DEBUG && defined(HAVE_VLD)
#  include <vld.h>
#endif

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	Controller controller;
	controller.show();
	return a.exec();
}
