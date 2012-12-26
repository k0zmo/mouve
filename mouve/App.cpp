#include <QApplication>
#include "NodeController.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	NodeController controller;
	controller.initSampleScene();
	return a.exec();
}
