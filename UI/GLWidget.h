#pragma once

#include "Prerequisites.h"

#include <QGLWidget>

namespace cv {
class Mat;
}
class QImage;

enum class EResizeBehavior
{
	FitToView,
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

	void fitInView();
	void scaleToOriginalSize(); 

	void zoomIn();
	void zoomOut();
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