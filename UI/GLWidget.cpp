#include "GLWidget.h"

#include <opencv2/core/core.hpp>

GLWidget::GLWidget(QWidget* parent)
	: QGLWidget(parent)
	, _left(0)
	, _bottom(0)
	, _right(1)
	, _top(1)
	, _textureId(0)
	, _textureCheckerId(0)
	, _foreignTextureId(0)
	, _textureWidth(0)
	, _textureHeight(0)
	, _resizeBehavior(EResizeBehavior::Scale)
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
		return;

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

	_textureWidth = image.width();
	_textureHeight = image.height();
	_showDummy = false;

	recalculateTexCoords();
	updateGL();
}

void GLWidget::show(const cv::Mat& image)
{
	if(!image.data || image.rows == 0 || image.cols == 0)
		return;

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

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.cols, image.rows,
		0, format, type, image.data);

	_textureWidth = image.cols;
	_textureHeight = image.rows;
	_showDummy = false;

	recalculateTexCoords();
	updateGL();
}

void GLWidget::showDummy()
{
	_showDummy = true;

	updateGL();
}

void GLWidget::setResizeBehavior(EResizeBehavior behavior)
{
	_resizeBehavior = behavior;
	updateGL();
}

void GLWidget::fitInView()
{
	_resizeBehavior = EResizeBehavior::Scale;

	if(_showDummy)
		return;

	recalculateTexCoords();
	updateGL();
}

void GLWidget::scaleToOriginalSize()
{
	_resizeBehavior = EResizeBehavior::MaintainImageSize;

	if(_showDummy)
		return;

	recalculateTexCoords();
	updateGL();
}


void GLWidget::zoomIn()
{
	zoom(1, 0.15f);
}

void GLWidget::zoomOut()
{
	zoom(-1, 0.15f);
}

void GLWidget::zoom(int dir, float scale)
{
	const float maxZoom = 0.05f;

	float distx = fabsf(_right - _left);
	float disty = fabsf(_top - _bottom);

	float distx1 = qBound(maxZoom, distx - scale * dir, 1.0f);
	float disty1 = qBound(maxZoom, disty - scale * dir, 1.0f);

	float dx = (-distx1 + distx) * 0.5f;
	float dy = (-disty1 + disty) * 0.5f;

	_left += dx;
	_right -= dx;
	_top -= dy;
	_bottom += dy;
 
	// TODO: This could cause some coords to be too much corrected
	if(_right > 1.0f)
	{
		_left = _left - (_right - 1.0f);
		_right = 1.0f;
	}
	if(_left < 0.0f)
	{
		_right = _right - _left;
		_left = 0.0f;		
	}
	if(_top > 1.0f)
	{
		_bottom = _bottom - (_top - 1.0f);
		_top = 1.0f;
	}
	if(_bottom < 0.0f)
	{
		_top = _top - _bottom;
		_bottom = 0.0f;
	}

	updateGL();
}

void GLWidget::recalculateTexCoords()
{
	if(_resizeBehavior == EResizeBehavior::Scale)
	{
		_left = 0;
		_bottom = 0;
		_right = 1;
		_top = 1;
	}
	else if(_resizeBehavior == EResizeBehavior::MaintainImageSize)
	{
		QSize widgetSize = size();

		int width = widgetSize.width();
		int height = widgetSize.height();

		_left = (_textureWidth - width) * 0.5f * (1.0f / float(_textureWidth));
		_right = (_textureWidth + width) * 0.5f * (1.0f / float(_textureWidth));

		_bottom = (_textureHeight - height) * 0.5f * (1.0f / float(_textureHeight));
		_top = (_textureHeight + height) * 0.5f * (1.0f / float(_textureHeight));
	}
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

	if(_resizeBehavior == EResizeBehavior::MaintainImageSize 
		&& !_showDummy)
		scaleToOriginalSize();
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
		for(int i = 1; i < sizeof(st) / sizeof(float); i += 2)
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

	if(_resizeBehavior == EResizeBehavior::MaintainImageSize)
		return;

	int dir = event->delta() > 0 ? 1 : -1;

	float c = 0.05f;
	if(event->modifiers() & Qt::ControlModifier)
		c *= 5.0f;
	
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

	float area = (_right - _left) * (_top - _bottom);
	if(area > 1.0f)
		return;

	float c = 0.001f;// * area; // connect with current zoom level 
	
	int dx = -diff.x();
	int dy = +diff.y();

	if(_top > 1.0f && _bottom < 0.0f)
		dy = 0;
	if(_right > 1.0f && _left < 0.0f)
		dx = 0;

	// translate to right 
	if(dx > 0)
	{
		float right = qBound(0.0f, _right + c * dx, 1.0f);
		_left = _left - (_right - right);
		_right = right;
	}
	// translate to left
	else if(dx < 0)
	{
		float left = qBound(0.0f, _left + c * dx, 1.0f);
		_right = _right - (_left - left);
		_left = left;
	}

	// translate to top
	if(dy > 0)
	{
		float top = qBound(0.0f, _top + c * dy, 1.0f);
		_bottom = _bottom - (_top - top);
		_top = top;
	}
	// translate to top
	else if(dy < 0)
	{
		float bottom = qBound(0.0f, _bottom + c * dy, 1.0f);
		_top = _top - (_bottom - bottom);
		_bottom = bottom;
	}

	updateGL();
}