#include "JaiNodeModule.h"
#include "JaiException.h"

#if defined(HAVE_JAI)

JaiNodeModule::JaiNodeModule()
	: _factoryHandle(nullptr)
{
}

JaiNodeModule::~JaiNodeModule()
{
	J_STATUS_TYPE error;

	for(CAM_HANDLE& camHandle : _camHandles)
	{
		if(!camHandle)
			continue;
		if((error = J_Camera_Close(camHandle)) != J_ST_SUCCESS)
		{}// Doesn't make any sense to do anything here
		camHandle = nullptr;
	}

	if(_factoryHandle)
	{
		if((J_Factory_Close(_factoryHandle)) != J_ST_SUCCESS)
		{}// Doesn't make any sense to do anything here
		_factoryHandle = nullptr;
	}
}

bool JaiNodeModule::initialize()
{
	int8_t psPrivateData[1] = {0};
	J_STATUS_TYPE error;
		
	if((error = J_Factory_Open(psPrivateData, &_factoryHandle)) != J_ST_SUCCESS)
		return false; //throw JaiException(error, "Factory couldn't be opened.");
	bool8_t hasChange;
	if((error = J_Factory_UpdateCameraList(_factoryHandle, &hasChange)) != J_ST_SUCCESS)
		return false; // throw JaiException(error, "Cameras list couldn't be updated");
	// Get number of discovered cameras
	uint32_t numDevices;
	if((error = J_Factory_GetNumOfCameras(_factoryHandle, &numDevices)) != J_ST_SUCCESS)
		return false; //throw JaiException(error, "Couldn't get the number of discovered cameras");
	if(numDevices > 0)
		_camHandles.resize(numDevices);

	return numDevices > 0;
}

bool JaiNodeModule::ensureInitialized()
{
	if(!_factoryHandle)
		return initialize();
	return true;
}

std::string JaiNodeModule::moduleName() const
{
	return "jai";
}

CAM_HANDLE JaiNodeModule::openCamera(int index)
{
	if(!_factoryHandle)
		throw JaiException(J_ST_INVALID_HANDLE, "Invalid factory handle");
	if(size_t(index) >= (int)_camHandles.size())
		throw JaiException(J_ST_INVALID_ID, "Invalid camera Id");

	// Get camera ID
	J_STATUS_TYPE error;
	int8_t cameraId[J_CAMERA_ID_SIZE];
	uint32_t size = uint32_t(sizeof(cameraId));

	if((error = J_Factory_GetCameraIDByIndex(_factoryHandle, index, cameraId, &size)) != J_ST_SUCCESS)
		throw JaiException(error, "Camera Id couldn't be retrieved");

	auto& camHandle = _camHandles[index];
		
	// Open camera
	if(!camHandle)
	{
		if ((error = J_Camera_Open(_factoryHandle, cameraId, &camHandle)) != J_ST_SUCCESS)
			throw JaiException(error, "Selected camera couldn't be opened");
	}

	return camHandle;
}

void JaiNodeModule::closeCamera(CAM_HANDLE camHandle)
{
	for(auto& cam : _camHandles)
	{
		if(cam == camHandle)
		{
			J_STATUS_TYPE error;
			if((error = J_Camera_Close(camHandle)) != J_ST_SUCCESS)
			{}// Doesn't make any sense to do anything here

			cam = nullptr;
			camHandle = nullptr;

			return;
		}
	}		
}

template<>
int64_t queryNodeValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName)
{
	int64_t value;
	J_STATUS_TYPE retval = J_Camera_GetValueInt64(hCamera, (int8_t*)nodeName, &value);
	if (retval != J_ST_SUCCESS)
		throw JaiException(retval);
	return value;
}

#endif