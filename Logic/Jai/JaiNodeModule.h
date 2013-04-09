#pragma once

#if defined(HAVE_JAI)

#include "Logic/NodeModule.h"

#include <Jai_Factory.h>

/// TODO: 
/// * Support for 10-bit bayer formats
/// * JaiDynamicFactory
/// * White-balance RGB gain finder
/// * Quering pixel format, width, height and adc gain values for properties (LUTSample)
/// * (LONG) Use lower primitives (StreamThread instead of StreamCallback)

class LOGIC_EXPORT JaiNodeModule : public NodeModule
{
public:
	JaiNodeModule();
	~JaiNodeModule() override;

	bool initialize() override;
	bool ensureInitialized() override;

	std::string moduleName() const override;

	CAM_HANDLE openCamera(int index);
	void closeCamera(CAM_HANDLE camHandle);

private:
	FACTORY_HANDLE _factoryHandle;
	std::vector<CAM_HANDLE> _camHandles;
};
	
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