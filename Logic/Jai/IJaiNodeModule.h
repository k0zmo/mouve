#pragma once

#include "Logic/NodeModule.h"

template<typename T>
struct RangedValue
{
	RangedValue() : minValue(0), maxValue(0), value(0) {}
	RangedValue(T minValue, T maxValue, T value)
		: minValue(minValue), maxValue(maxValue), value(value) {}

	T minValue;
	T maxValue;
	T value;
};

struct CameraSettings
{
	RangedValue<int64_t> offsetX;
	RangedValue<int64_t> offsetY;
	RangedValue<int64_t> width;
	RangedValue<int64_t> height;
	RangedValue<int64_t> gain;
	int64_t pixelFormat;
	std::vector<std::tuple<int64_t, std::string>> pixelFormats;	
};

struct CameraInfo
{
	std::string id;
	std::string modelName;
	std::string manufacturer;
	std::string interfaceId;
	std::string ipAddress;
	std::string macAddress;
	std::string serialNumber;
	std::string userName;
};

enum class EDriverType { All, Filter, Socket };

class LOGIC_EXPORT IJaiNodeModule : public NodeModule
{
public:
	virtual int cameraCount() const = 0;
	virtual std::vector<CameraInfo> discoverCameras(EDriverType driverType = EDriverType::Filter) = 0;
	virtual CameraSettings cameraSettings(int index) = 0;
	virtual bool setCameraSettings(int index, const CameraSettings& settings) = 0;
};

// Will return null if Logic wasn't compiled with support of JAI cameras
LOGIC_EXPORT std::unique_ptr<IJaiNodeModule> createJaiModule();