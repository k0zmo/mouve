#pragma once

#include "Prerequisites.h"

#include <QWidget>

namespace Ui {
	class PreviewWidget;
}

class PreviewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit PreviewWidget(QWidget* parent = nullptr);
	~PreviewWidget() override;

public slots:
	void show(const QImage& image);
	void show(const cv::Mat& image);
	void showDummy();

private slots:
	void changeBehavior(bool checked);

private:
	void setToolbarEnabled(bool enabled);

private:
	Ui::PreviewWidget* _ui;
};