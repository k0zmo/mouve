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

#include "PreviewWidget.h"

#include "ui_PreviewWidget.h"

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
    , _ui(new Ui::PreviewWidget())
{
    _ui->setupUi(this);

    connect(_ui->toolButtonBehavior, &QToolButton::clicked,
        this, &PreviewWidget::changeBehavior);
    connect(_ui->toolButtonZoomIn, &QToolButton::clicked,
        _ui->glwidget, &GLWidget::zoomIn);
    connect(_ui->toolButtonZoomOut, &QToolButton::clicked,
        _ui->glwidget, &GLWidget::zoomOut);
    connect(_ui->toolButtonWholeImage, &QToolButton::clicked,
        _ui->glwidget, &GLWidget::zoomFitWhole);
    connect(_ui->toolButtonOriginal, &QToolButton::clicked,
        _ui->glwidget, &GLWidget::zoomOriginal);
}

PreviewWidget::~PreviewWidget()
{
    delete _ui;
}

void PreviewWidget::show(const QImage& image)
{
    setToolbarEnabled(true);
    _ui->glwidget->show(image);
}

void PreviewWidget::show(const cv::Mat& image)
{
    setToolbarEnabled(true);
    _ui->glwidget->show(image);
}

void PreviewWidget::showDummy()
{
    setToolbarEnabled(false);
    _ui->glwidget->showDummy();
}

void PreviewWidget::updateInformation(const QString& str)
{
    _ui->plainTextEdit->setPlainText(str);
}

void PreviewWidget::changeBehavior(bool checked)
{
    if(!checked)
    {
        _ui->glwidget->fitWithAspect();
    }
    else
    {
        _ui->glwidget->fitWithWidget();	
    }

    _ui->toolButtonOriginal->setEnabled(!checked);
}

void PreviewWidget::setToolbarEnabled(bool enabled)
{
    _ui->toolButtonBehavior->setEnabled(enabled);
    _ui->toolButtonZoomIn->setEnabled(enabled);
    _ui->toolButtonZoomOut->setEnabled(enabled);
    _ui->toolButtonWholeImage->setEnabled(enabled);

    bool zoomEnabled = enabled && !_ui->toolButtonBehavior->isChecked();
    _ui->toolButtonOriginal->setEnabled(zoomEnabled);
}
