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

#include "Prerequisites.h"

#include <QGLWidget>

namespace cv {
class Mat;
}
class QImage;

enum class EResizeBehavior
{
    FitWithWidget,
    MaintainAspectRatio
};

class GLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    virtual ~GLWidget();

public slots:
    void show(const QImage& image);
    void show(const cv::Mat& image);
    void showDummy();
    /// TODO: void show(GLuint texture);

    void fitWithWidget();
    void fitWithAspect(); 

    void zoomIn();
    void zoomOut();
    void zoomFitWhole();
    void zoomOriginal();

private:
    void zoom(int dir, qreal scale);
    void move(int dx, int dy);
    void recalculateTexCoords();
    void setDefaultSamplerParameters();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    float _top;
    float _left;
    float _right;
    float _bottom;

    qreal _topBase;
    qreal _leftBase;
    qreal _rightBase;
    qreal _bottomBase;

    GLuint _textureId;
    GLuint _foreignTextureId;
    GLuint _textureCheckerId;
    int _textureWidth;
    int _textureHeight;

    QPoint _mouseAnchor;

    EResizeBehavior _resizeBehavior;
    bool _showDummy;
};