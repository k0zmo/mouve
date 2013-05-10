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

void PreviewWidget::changeBehavior(bool checked)
{
	if(!checked)
	{
		_ui->glwidget->scaleToOriginalSize();
	}
	else
	{
		_ui->glwidget->fitInView();
	}
}

void PreviewWidget::setToolbarEnabled(bool enabled)
{
	_ui->toolButtonBehavior->setEnabled(enabled);
	_ui->toolButtonOriginal->setEnabled(enabled);
	_ui->toolButtonZoomIn->setEnabled(enabled);
	_ui->toolButtonZoomOut->setEnabled(enabled);
}
