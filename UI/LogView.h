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

#pragma once

#include <QListWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <qlogging.h>

class LogView : public QWidget
{
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = nullptr);
    ~LogView() override;

public slots:
    void debug(const QString& msg);
    void warn(const QString& msg);
    void critical(const QString& msg);

private:
    static void messageHandler(QtMsgType type, 
        const QMessageLogContext& context, const QString& msg);

private:
    QHBoxLayout* _layout;
    QListWidget* _widget;

    // Intrusive list of all created log views (in distant future: all log channels)
    LogView* _nextView;
    static LogView* _headView;
    static QIcon _warningIcon;
    static QIcon _infoIcon;
    static QIcon _errorIcon;
};

