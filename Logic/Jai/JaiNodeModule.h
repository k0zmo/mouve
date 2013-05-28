#pragma once

#include "IJaiNodeModule.h"

#if defined(HAVE_JAI)

#include "Logic/NodeModule.h"

#include <Jai_Factory.h>

using std::string;
using std::vector;
using std::tuple;

/// TODO: 
/// * Support for 10-bit bayer formats
/// * White-balance RGB gain finder
/// * (LONG) Use lower primitives (StreamThread instead of StreamCallback)

class LOGIC_EXPORT JaiNodeModule : public IJaiNodeModule
{
public:
	JaiNodeModule();
	~JaiNodeModule() override;

	bool initialize() override;
	bool isInitialized() override;

	string moduleName() const override;

	int cameraCount() const override { return int(_camHandles.size()); }
	vector<CameraInfo> discoverCameras(EDriverType driverType = EDriverType::Filter) override;
	CameraSettings cameraSettings(int index) override;
	bool setCameraSettings(int index, const CameraSettings& settings)  override;

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
