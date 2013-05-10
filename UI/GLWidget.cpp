#include "GLWidget.h"

#include <opencv2/core/core.hpp>
#include <QImage>
#include <QWheelEvent>

GLWidget::GLWidget(QWidget* parent)
	: QGLWidget(parent)
	, _top(1)
	, _left(0)
	, _right(1)
	, _bottom(0)
	, _topBase(1)
	, _leftBase(0)
	, _rightBase(1)
	, _bottomBase(0)
	, _textureId(0)
	, _foreignTextureId(0)
	, _textureCheckerId(0)
	, _textureWidth(0)
	, _textureHeight(0)
	, _resizeBehavior(EResizeBehavior::MaintainAspectRatio)
	, _showDummy(true)
{
}

GLWidget::~GLWidget()
{
	glDeleteTextures(1, &_textureId);
	glDeleteTextures(1, &_textureCheckerId);
}

void GLWidget::show(const QImage& image)
{
	if(image.isNull())
	{
		showDummy();
		return;
	}

	glBindTexture(GL_TEXTURE_2D, _textureId);

	setDefaultSamplerParameters();

	GLenum format;
	GLenum internalFormat;
	GLenum type;

	switch(image.format())
	{
	case QImage::Format_Indexed8:
		// TODO: We assume it's simple gray colormap
		format = GL_RED;
		internalFormat = GL_R8;
		type = GL_UNSIGNED_BYTE;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;

	case QImage::Format_RGB32:
		format = GL_BGRA;
		internalFormat = GL_RGB8;
		type = GL_UNSIGNED_BYTE;
		break;

	case QImage::Format_ARGB32:
		format = GL_BGRA;
		internalFormat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	
	case QImage::Format_RGB16:
		internalFormat = GL_RGB8;
		format = GL_RGB;
		type = GL_UNSIGNED_SHORT_5_6_5;
		break;

	case QImage::Format_RGB555:
		internalFormat = GL_RGBA8;
		format = GL_BGRA;
		type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;

	case QImage::Format_RGB888:
		internalFormat = GL_RGB8;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;

	case QImage::Format_Invalid:
	case QImage::Format_Mono:
	case QImage::Format_MonoLSB:
	case QImage::Format_RGB666:
	case QImage::Format_RGB444:
	case QImage::Format_ARGB32_Premultiplied:
	case QImage::Format_ARGB8565_Premultiplied:
	case QImage::Format_ARGB6666_Premultiplied:
	case QImage::Format_ARGB8555_Premultiplied:
	case QImage::Format_ARGB4444_Premultiplied:
		// TODO: Some of them we could support in near future
		return;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width(), image.height(),	
		0, format, type, image.constBits());

	bool sizeChanged = _textureWidth != image.width()
		|| _textureHeight != image.height();

	_textureWidth = image.width();
	_textureHeight = image.height();
	_showDummy = false;

	if(sizeChanged)
		recalculateTexCoords();
	updateGL();
}

void GLWidget::show(const cv::Mat& image)
{
	if(!image.data || image.rows == 0 || image.cols == 0)
	{
		showDummy();
		return;
	}

	glBindTexture(GL_TEXTURE_2D, _textureId);

	setDefaultSamplerParameters();

	GLenum format = GL_INVALID_ENUM;
	GLenum internalFormat = GL_INVALID_ENUM;
	GLenum type = GL_INVALID_ENUM;

	switch(image.depth())
	{
	case CV_8U:
		switch(image.channels())
		{
		case 1:
			format = GL_RED;
			internalFormat = GL_R8;
			type = GL_UNSIGNED_BYTE;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			break;

		case 3:
			format = GL_BGR;
			internalFormat = GL_RGB8;
			type = GL_UNSIGNED_BYTE;
			break;

		case 4:
			break;
		}
		break;
	case CV_8S:
	case CV_16U:
	case CV_16S:
	case CV_32S:
	case CV_32F:
	case CV_64F:
		break;
	}

	if(format == GL_INVALID_ENUM 
		|| internalFormat == GL_INVALID_ENUM
		|| type == GL_INVALID_ENUM)
	{
		return;
	}

	if(size_t(image.cols) != image.step)
	{
		// TODO: Use PBO? or something else to skip pointless copying because of step size
		char* data = new char[image.elemSize() * image.rows * image.cols];
		
		for(int y = 0; y < image.rows; ++y)
			memcpy(data + y * image.cols * image.elemSize(),
				image.data + y * image.step, image.elemSize() * image.cols);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.cols, image.rows,
			0, format, type, data);

		delete [] data;
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.cols, image.rows,
			0, format, type, image.data);
	}

	bool sizeChanged = _textureWidth != image.cols
		|| _textureHeight != image.rows;

	_textureWidth = image.cols;
	_textureHeight = image.rows;
	_showDummy = false;

	if(sizeChanged)
		recalculateTexCoords();
	updateGL();
}

void GLWidget::showDummy()
{
	_showDummy = true;

	updateGL();
}

void GLWidget::fitInView()
{
	_resizeBehavior = EResizeBehavior::FitToView;

	if(_showDummy)
		return;

	recalculateTexCoords();
	updateGL();
}

void GLWidget::scaleToOriginalSize()
{
	_resizeBehavior = EResizeBehavior::MaintainAspectRatio;

	if(_showDummy)
		return;

	recalculateTexCoords();
	updateGL();
}

void GLWidget::zoomIn()
{
	zoom(1, 0.15);
}

void GLWidget::zoomOut()
{
	zoom(-1, 0.15);
}

void GLWidget::zoomOriginal()
{
	recalculateTexCoords();
	updateGL();
}

void GLWidget::zoom(int dir, qreal scale)
{
	// use rather varying max zoom, depending on image size
	// Old version: const qreal maxZoom = 0.05f;
	const qreal maxZoom = 20.0f / qMax(_textureHeight, _textureHeight);

	qreal distx = fabsf(_right - _left);
	qreal disty = fabsf(_top - _bottom);

	if(_resizeBehavior == EResizeBehavior::FitToView)
	{
		qreal distx1 = qBound(maxZoom, distx - scale * dir, 1.0);
		qreal disty1 = qBound(maxZoom, disty - scale * dir, 1.0);

		qreal dx = (-distx1 + distx) * 0.5;
		qreal dy = (-disty1 + disty) * 0.5;

		_left += dx;
		_right -= dx;
		_top -= dy;
		_bottom += dy;
	}
	else if(_resizeBehavior == EResizeBehavior::MaintainAspectRatio)
	{
		// we could check which aspect (y/x or x/y) is less than 1.0 and use that
		// but because 95% of images are not portrait type we can assume y < x in most cases
		// and make it much simpler (note inverted fractional)
		const qreal aspect = distx / disty;
		
		const qreal maxw = _rightBase - _leftBase;
		const qreal maxh = _topBase - _bottomBase;

		const qreal distx1 = qBound(maxZoom, distx - aspect * scale * dir, maxw);
		const qreal disty1 = qBound(maxZoom, disty - scale * dir, maxh);

		const qreal aspect1 = distx1 / disty1;
		qreal dx, dy;

		// Check if last op. changed aspect ratio
		if(!qFuzzyCompare(aspect, aspect1))
		{
			qreal a = distx - (dir > 0 ? maxZoom : maxw);
			qreal b = disty - (dir > 0 ? maxZoom : maxh);

			dx = aspect * qMin(a,b) * 0.5;
			dy = qMin(a,b) * 0.5;
		}
		else
		{
			dx = (distx - distx1) * 0.5;
			dy = (disty - disty1) * 0.5;
		}

		_left += dx;
		_right -= dx;
		_top -= dy;
		_bottom += dy;
	}

	if(_right > _rightBase)
	{
		_left = _left - (_right - _rightBase);
		_right = _rightBase;
	}
	if(_left < _leftBase)
	{
		_right = _right - (_left - _leftBase);
		_left = _leftBase;		
	}
	if(_top > _topBase)
	{
		_bottom = _bottom - (_top - _topBase);
		_top = _topBase;
	}
	if(_bottom < _bottomBase)
	{
		_top = _top - (_bottom - _bottomBase);
		_bottom = _bottomBase;
	}

	updateGL();
}

void GLWidget::move(int dx, int dy)
{
	// when image is smaller than a widget
	qreal area = (_right - _left) * (_top - _bottom);
	//if(area >= 1.0)
	//	return;

	static const qreal c = 0.001;
	// watch out for log(1) == 0
	//const qreal c = 0.001f / log((_rightBase - _leftBase) / (_right - _left));

	if(_top > _topBase && _bottom < _bottomBase)
		dy = 0;
	if(_right > _rightBase && _left < _leftBase)
		dx = 0;

	// translate to right 
	if(dx > 0)
	{
		qreal right = qBound(_leftBase, _right + c * dx, _rightBase);
		_left = _left - (_right - right);
		_right = right;
	}
	// translate to left
	else if(dx < 0)
	{
		qreal left = qBound(_leftBase, _left + c * dx, _rightBase);
		_right = _right - (_left - left);
		_left = left;
	}

	// translate to top
	if(dy > 0)
	{
		qreal top = qBound(_bottomBase, _top + c * dy, _topBase);
		_bottom = _bottom - (_top - top);
		_top = top;
	}
	// translate to top
	else if(dy < 0)
	{
		qreal bottom = qBound(_bottomBase, _bottom + c * dy, _topBase);
		_top = _top - (_bottom - bottom);
		_bottom = bottom;
	}

	updateGL();
}

void GLWidget::recalculateTexCoords()
{
	if(_resizeBehavior == EResizeBehavior::FitToView)
	{
		_leftBase = 0;
		_bottomBase = 0;
		_rightBase = 1;
		_topBase = 1;
	}
	else if(_resizeBehavior == EResizeBehavior::MaintainAspectRatio)
	{
		QSize widgetSize = size();

		int width = widgetSize.width();
		int height = widgetSize.height();

		qreal waspect = width / (qreal) _textureWidth;
		qreal haspect = height / (qreal) _textureHeight;

		qreal w = _textureWidth * qMin(waspect, haspect);
		qreal h = _textureHeight * qMin(waspect, haspect);

		_leftBase = (w - width) * 0.5 * (1.0 / w);
		_rightBase = (w + width) * 0.5 * (1.0 / w);
		_bottomBase = (h - height) * 0.5 * (1.0 / h);
		_topBase = (h + height) * 0.5 * (1.0 / h);
	}

	_left = _leftBase;
	_right = _rightBase;
	_top = _topBase;
	_bottom = _bottomBase;
}

void GLWidget::setDefaultSamplerParameters()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Not entirely sampler parameters :) At least according to OpenGL docs
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
}

void GLWidget::initializeGL()
{
	// just basic set up
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glGenTextures(1, &_textureId);

	glGenTextures(1, &_textureCheckerId);
	unsigned char sqData[32*32*3];
	for(int y = 0; y < 32; ++y)
	{
		for(int x = 0; x < 32*3; x+=3)
		{
			if((y < 16 && x < 16*3) || (y >= 16 && x >= 16*3))
			{
				sqData[x + y*32*3 + 0] = 200;
				sqData[x + y*32*3 + 1] = 200;
				sqData[x + y*32*3 + 2] = 200;
			}
			else
			{
				sqData[x + y*32*3 + 0] = 100;
				sqData[x + y*32*3 + 1] = 100;
				sqData[x + y*32*3 + 2] = 100;
			}
		}
	}

	glGenTextures(1, &_textureCheckerId);
	glBindTexture(GL_TEXTURE_2D, _textureCheckerId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, sqData);

	glEnable(GL_TEXTURE_2D);

	// Deprecated
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GLWidget::resizeGL(int width, int height)
{
	if(height == 0)
		height = 1;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(!_showDummy && _resizeBehavior == EResizeBehavior::MaintainAspectRatio)
		recalculateTexCoords();
}

void GLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor3f(1, 1, 1);

	float pos[] = {
		-1, +1, 0,
		-1, -1, 0,
		+1, +1, 0,
		+1, -1, 0
	};

	if(_showDummy)
	{
		float st[] = {
			00, 00,
			00, 16,
			16, 00,
			16, 16
		};

		glBindTexture(GL_TEXTURE_2D, _textureCheckerId);
		glVertexPointer(3, GL_FLOAT, 0, pos);
		glTexCoordPointer(2, GL_FLOAT, 0, st);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	else
	{
		float st[] = {
			_left,  _top,
			_left,  _bottom,
			_right, _top,
			_right, _bottom
		};

		// Flip texture horizontally
		for(size_t i = 1; i < sizeof(st) / sizeof(float); i += 2)
			st[i] = 1.0f - st[i];

		glBindTexture(GL_TEXTURE_2D, _textureId);
		glVertexPointer(3, GL_FLOAT, 0, pos);
		glTexCoordPointer(2, GL_FLOAT, 0, st);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void GLWidget::wheelEvent(QWheelEvent* event)
{
	if(_showDummy)
		return;

	int dir = event->delta() > 0 ? 1 : -1;

	qreal c = 0.05;
	if(event->modifiers() & Qt::ControlModifier)
		c *= 5.0;
	
	zoom(dir, c);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
	_mouseAnchor = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
	if(_showDummy)
		return;

	QPoint diff = event->pos() - _mouseAnchor;
	_mouseAnchor = event->pos();

	move(-diff.x(), +diff.y());
}
