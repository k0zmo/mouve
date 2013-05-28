#include "JaiNodeModule.h"
#include "JaiException.h"

#if defined(HAVE_JAI)

// Needed for PathIsDirectory() used in Jai_Factory_Dynamic.h
#pragma comment(lib, "ShLwApi.lib")
// Needed for _T() macros and other _t* macros
#include <tchar.h>
#include "Jai_Factory_Dynamic.h"

template<typename T> void setNodeValue(CAM_HANDLE hCamera, const char* nodeName, T value);
template<> void setNodeValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName, int64_t value);
template<typename T> RangedValue<T> queryNodeRangedValue(CAM_HANDLE hCamera, const char* nodeName);
template<> RangedValue<int64_t> queryNodeRangedValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName);
vector<tuple<int64_t, string>> queryNodeEnumValue(CAM_HANDLE hCamera, const char* nodeName);

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

	return true; //numDevices > 0;
}

bool JaiNodeModule::isInitialized()
{
	return _factoryHandle != nullptr;
}

string JaiNodeModule::moduleName() const
{
	return "jai";
}

vector<CameraInfo> JaiNodeModule::discoverCameras(EDriverType driverType)
{
	vector<CameraInfo> list;
	if(!ensureInitialized())
		return list;

	// In fact, cameras are already discovered, we just need to query for their names
	for(auto iter = _camHandles.begin(), end = _camHandles.end(); iter != end; ++iter)
	{
		J_STATUS_TYPE error;
		int8_t cameraId[J_CAMERA_ID_SIZE];
		uint32_t size = uint32_t(sizeof(cameraId));

		if((error = J_Factory_GetCameraIDByIndex(_factoryHandle, 
			iter - _camHandles.begin(), cameraId, &size)) != J_ST_SUCCESS)
		{
			continue;
		}

		// Get iD name
		CameraInfo cameraInfo;
		cameraInfo.id = string(reinterpret_cast<char*>(cameraId));

		int8_t buffer[J_CAMERA_INFO_SIZE];
		auto queryCameraInfo = [&](int8_t cameraId[], 
			J_CAMERA_INFO camInfo) -> string
			{
				uint32_t size = uint32_t(sizeof(buffer));
				if((error = J_Factory_GetCameraInfo(_factoryHandle, cameraId,
					camInfo, buffer, &size)) != J_ST_SUCCESS)
				{
					return string();
				}
				return string(reinterpret_cast<char*>(buffer));
			};

		cameraInfo.modelName = queryCameraInfo(cameraId, CAM_INFO_MODELNAME);
		if(cameraInfo.id.find("INT=>FD") != string::npos)
		{
			if(driverType != EDriverType::Socket)
				cameraInfo.modelName += " (Filter driver)";
			else
				continue;
		}
		else if(cameraInfo.id.find("INT=>SD") != string::npos)
		{
			if(driverType != EDriverType::Filter)
				cameraInfo.modelName += " (Socket driver)";
			else
				continue;
		}

		cameraInfo.manufacturer = queryCameraInfo(cameraId, CAM_INFO_MANUFACTURER);
		cameraInfo.interfaceId = queryCameraInfo(cameraId, CAM_INFO_INTERFACE_ID);
		cameraInfo.ipAddress = queryCameraInfo(cameraId, CAM_INFO_IP);
		cameraInfo.macAddress = queryCameraInfo(cameraId, CAM_INFO_MAC);
		cameraInfo.serialNumber = queryCameraInfo(cameraId, CAM_INFO_SERIALNUMBER);
		cameraInfo.userName = queryCameraInfo(cameraId, CAM_INFO_USERNAME);		
		list.emplace_back(std::move(cameraInfo));
	}

	return list;
}


CameraSettings JaiNodeModule::cameraSettings(int index)
{
	CameraSettings set;
	if(!ensureInitialized())
		return set;

	if(size_t(index) >= (int)_camHandles.size())
		throw JaiException(J_ST_INVALID_ID, "Invalid camera Id");

	try
	{
		auto& camHandle = _camHandles[index];
		bool tmpHandle = false;
		
		// Open camera if necessary
		if(!camHandle)
		{
			camHandle = openCamera(index);
			tmpHandle = true;
		}

		set.offsetX = queryNodeRangedValue<int64_t>(camHandle, NODE_NAME_OFFSETX);
		set.offsetY = queryNodeRangedValue<int64_t>(camHandle, NODE_NAME_OFFSETY);
		set.width = queryNodeRangedValue<int64_t>(camHandle, NODE_NAME_WIDTH);
		set.height = queryNodeRangedValue<int64_t>(camHandle, NODE_NAME_HEIGHT);
		set.gain = queryNodeRangedValue<int64_t>(camHandle, NODE_NAME_GAIN);
		set.pixelFormat = queryNodeValue<int64_t>(camHandle, NODE_NAME_PIXELFORMAT);
		set.pixelFormats = queryNodeEnumValue(camHandle, NODE_NAME_PIXELFORMAT);

		if(tmpHandle)
			closeCamera(camHandle);

		return set;
	}
	catch(JaiException&)
	{
		return set;
	}
}

bool JaiNodeModule::setCameraSettings(int index, const CameraSettings& settings)
{
	if(!ensureInitialized())
		return false;

	if(size_t(index) >= (int)_camHandles.size())
		throw JaiException(J_ST_INVALID_ID, "Invalid camera Id");

	try
	{
		auto& camHandle = _camHandles[index];
		bool tmpHandle = false;
		
		// Open camera if necessary
		if(!camHandle)
		{
			camHandle = openCamera(index);
			tmpHandle = true;
		}

		setNodeValue<int64_t>(camHandle, NODE_NAME_OFFSETX, settings.offsetX.value);
		setNodeValue<int64_t>(camHandle, NODE_NAME_OFFSETY, settings.offsetY.value);
		setNodeValue<int64_t>(camHandle, NODE_NAME_WIDTH, settings.width.value);
		setNodeValue<int64_t>(camHandle, NODE_NAME_HEIGHT, settings.height.value);
		setNodeValue<int64_t>(camHandle, NODE_NAME_GAIN, settings.gain.value);
		setNodeValue<int64_t>(camHandle, NODE_NAME_PIXELFORMAT, settings.pixelFormat);

		if(tmpHandle)
			closeCamera(camHandle);

		return true;
	}
	catch(JaiException&)
	{
		return false;
	}
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
		throw JaiException(retval, "Couldn't retrieve Int64 value");
	return value;
}

template<typename T> 
void setNodeValue(CAM_HANDLE hCamera, const char* nodeName, T value) {}

template<>
void setNodeValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName, int64_t value)
{
	J_STATUS_TYPE retval = J_Camera_SetValueInt64(hCamera, (int8_t*)nodeName, value);
	if (retval != J_ST_SUCCESS)
		throw JaiException(retval, "Couldn't set Int64 value");
}

template<typename T>
RangedValue<T> queryNodeRangedValue(CAM_HANDLE hCamera, const char* nodeName)
{
	return RangedValue<T>();
}

template<>
RangedValue<int64_t> queryNodeRangedValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName)
{
	NODE_HANDLE hNode = nullptr;
	J_STATUS_TYPE retval;
	RangedValue<int64_t> rangedValue;

	if((retval = J_Camera_GetNodeByName(hCamera, 
		(int8_t*) nodeName, &hNode)) == J_ST_SUCCESS)
	{
		retval = J_Node_GetMinInt64(hNode, &rangedValue.minValue);
		retval = J_Node_GetMaxInt64(hNode, &rangedValue.maxValue);
		retval = J_Node_GetValueInt64(hNode, false, &rangedValue.value);
	}

	return rangedValue;
}

vector<tuple<int64_t, string>> queryNodeEnumValue(CAM_HANDLE hCamera, const char* nodeName)
{
	NODE_HANDLE hNode = nullptr;
	J_STATUS_TYPE retval;
	vector<tuple<int64_t, string>> enumValues;

	if((retval = J_Camera_GetNodeByName(hCamera, 
		(int8_t*)nodeName, &hNode)) == J_ST_SUCCESS)
	{
		uint32_t numEnums;
		if((retval = J_Node_GetNumOfEnumEntries(hNode,
			&numEnums)) == J_ST_SUCCESS)
		{
			for(uint32_t i = 0; i < numEnums; ++i)
			{
				NODE_HANDLE hEnumNode = nullptr;
				if((retval = J_Node_GetEnumEntryByIndex(hNode,
					i, &hEnumNode)) == J_ST_SUCCESS)
				{
					J_NODE_ACCESSMODE accessMode;
					if((retval =J_Node_GetAccessMode(hEnumNode,
						&accessMode)) == J_ST_SUCCESS
						&& (accessMode != NI)
						&& (accessMode != NA))
					{
						int8_t buffer[J_CONFIG_INFO_SIZE];
						uint32_t size = sizeof(buffer);

						if((retval = J_Node_GetDescription(hEnumNode,
							buffer, &size)) != J_ST_SUCCESS)
						{
							continue;
						}

						string desc = reinterpret_cast<char*>(buffer);
						int64_t value;

						if((retval = J_Node_GetEnumEntryValue(hEnumNode,
							&value)) != J_ST_SUCCESS)
						{
							continue;
						}

						
						enumValues.emplace_back(std::make_tuple(value, desc));
					}
				}
			}
		}
	}

	return enumValues;
}

std::unique_ptr<IJaiNodeModule> createJaiModule()
{
	return std::unique_ptr<IJaiNodeModule>(new JaiNodeModule());
}

#else

std::unique_ptr<IJaiNodeModule> createJaiModule()
{
	return nullptr;
}

#endif
