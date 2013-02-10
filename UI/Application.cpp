#include <QApplication>
#include "Controller.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	Controller controller;
	controller.show();
	return a.exec();
}
