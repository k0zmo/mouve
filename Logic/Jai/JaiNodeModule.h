#pragma once

#if defined(HAVE_JAI)

#include "Logic/NodeModule.h"

#include <Jai_Factory.h>

using std::vector;
using std::string;
using std::tuple;

/// TODO: 
/// * Support for 10-bit bayer formats
/// * JaiDynamicFactory
/// * White-balance RGB gain finder
/// * (LONG) Use lower primitives (StreamThread instead of StreamCallback)

struct CameraInfo
{
	string id;
	string modelName;
	string manufacturer;
	string interfaceId;
	string ipAddress;
	string macAddress;
	string serialNumber;
	string userName;
};

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
	vector<tuple<int64_t, string>> pixelFormats;	
};

enum class EDriverType { All, Filter, Socket };

class LOGIC_EXPORT JaiNodeModule : public NodeModule
{
public:
	JaiNodeModule();
	~JaiNodeModule() override;

	bool initialize() override;
	bool ensureInitialized() override;

	string moduleName() const override;

	int cameraCount() const { return int(_camHandles.size()); }
	vector<CameraInfo> discoverCameras(EDriverType driverType = EDriverType::Filter);
	CameraSettings cameraSettings(int index);
	bool setCameraSettings(int index, const CameraSettings& settings);

	CAM_HANDLE openCamera(int index);
	void closeCamera(CAM_HANDLE camHandle);

private:
	FACTORY_HANDLE _factoryHandle;
	vector<CAM_HANDLE> _camHandles;
};
	
#define NODE_NAME_OFFSETX "OffsetX"
#define NODE_NAME_OFFSETY "OffsetY"
#define NODE_NAME_WIDTH "Width"
#define NODE_NAME_HEIGHT "Height"
#define NODE_NAME_PIXELFORMAT "PixelFormat"
#define NODE_NAME_GAIN "GainRaw"
#define NODE_NAME_ACQSTART "AcquisitionStart"
#define NODE_NAME_ACQSTOP "AcquisitionStop"

template<typename T>
inline T queryNodeValue(CAM_HANDLE hCamera, const char* nodeName)
{ return T(); }

template<> int64_t queryNodeValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName);

#endif