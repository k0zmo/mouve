#include <QtGui/QApplication>

#include "NodeEditorView.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	NodeEditorView v;
	v.show();
	return a.exec();
}
