#pragma once

#include "ui_GpuChoice.h"

struct GpuPlatform;
enum class EDeviceType : int;

class GpuChoiceDialog : public QDialog, private Ui::GpuChoiceDialog
{
	Q_OBJECT
public:
	explicit GpuChoiceDialog(
		const std::vector<GpuPlatform>& gpuPlatforms, 
		QWidget* parent = nullptr);

	EDeviceType deviceType() const;
	int platformID() const;
	int deviceID() const;

public slots:
	void accept() override;

private slots:
	void changeDeviceTypeIndex(int index);

private:
	int extractUserData(int index) const;
};
