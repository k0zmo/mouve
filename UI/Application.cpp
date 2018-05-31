/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <QApplication>
#include <QGLFormat>
#include <QSettings>
#include "Controller.h"

#if defined(QT_DEBUG) && defined(HAVE_VLD)
#  include <vld.h>
#endif

#if defined(DEBUGGING_CONSOLE)
#  include <Windows.h>
#endif

#include <vector>

static const QString applicationTitle = QStringLiteral("mouve");

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName(applicationTitle);
    QCoreApplication::setOrganizationName(applicationTitle);

    QSettings settings;
    if (settings.value("defaultFont").toBool())
        QApplication::setFont(QFont());

#if defined(DEBUGGING_CONSOLE)
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    freopen("CON", "w", stdout);
#endif

    if (!QGLFormat::hasOpenGL())
    {
        qCritical("This system has no OpenGL support. Exiting.");
        return -1;
    }

    Controller controller;
    controller.show();
    return a.exec();
}
