#if defined(HAVE_JAI)

#include "NodeType.h"
#include "NodeFactory.h"

#include <Jai_Factory.h>
#include <opencv2/core/core.hpp>
#include <mutex>

#define NODE_NAME_WIDTH "Width"
#define NODE_NAME_HEIGHT "Height"
#define NODE_NAME_PIXELFORMAT "PixelFormat"
#define NODE_NAME_ACQSTART "AcquisitionStart"
#define NODE_NAME_ACQSTOP "AcquisitionStop"

/// TODO: 
/// * class JaiCamera
/// * Support for 10-bit bayer formats
/// * JaiModule
/// * JaiDynamicFactory
/// * White-balance RGB gain finder
/// * Quering pixel format, width, height and adc gain values for properties (LUTSample)
/// * (LONG) Use lower primitives (StreamThread instead of StreamCallback)

/*
class JaiNodeModule : public NodeModule
{
public:
	JaiNodeModule()
	{
	}

	~JaiNodeModule()
	{
	}
};
*/

class JaiCameraNodeType : public NodeType
{
public:
	JaiCameraNodeType()
		: _hFactory(nullptr)
		, _hCamera(nullptr)
		, _hStreamThread(nullptr)
		, _width(0)
		, _height(0)
	{
	}
	
	~JaiCameraNodeType() override
	{
		finish();

		J_STATUS_TYPE retval;

		if (_hCamera)
		{
			// Close camera
			retval = J_Camera_Close(_hCamera);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not close the camera!"), retval);
			}
			_hCamera = nullptr;
		}

		if (_hFactory)
		{
			// Close factory
			retval = J_Factory_Close(_hFactory);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not close the factory!"), retval);
			}
			_hFactory = nullptr;
		}
	}
	
	bool restart() override
	{
		J_STATUS_TYPE retval;
		
		// Open factory
		if(!_hFactory)
		{
			int8_t psPrivateData[1] = {0};
			retval = J_Factory_Open(psPrivateData, &_hFactory);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not open factory!"), retval);
				return false;
			}
			
			// Update camera list
			bool8_t hasChange;
			retval = J_Factory_UpdateCameraList(_hFactory, &hasChange);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not update camera list!"), retval);
				return false;
			}
			
			// Get the number of Cameras
			uint32_t numDevices;
			retval = J_Factory_GetNumOfCameras(_hFactory, &numDevices);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not get the number of cameras!"), retval);
				return false;
			}
			if (numDevices == 0)
			{
				//AfxMessageBox(CString("There is no camera!"), MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
			
			// Get camera ID
			int8_t cameraId[J_CAMERA_ID_SIZE];
			uint32_t size = uint32_t(sizeof(cameraId));

			retval = J_Factory_GetCameraIDByIndex(_hFactory, 0, cameraId, &size);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not get the camera ID!"), retval);
				return false;
			}

			// Open camera
			if(!_hCamera)
			{
				retval = J_Camera_Open(_hFactory, cameraId, &_hCamera);
				if (retval != J_ST_SUCCESS)
				{
					//ShowErrorMsg(CString("Could not open the camera!"), retval);
					return false;
				}
			}
		}
		
		int64_t int64Val;
		retval = J_Camera_GetValueInt64(_hCamera, (int8_t*)NODE_NAME_WIDTH, &int64Val);
		if (retval != J_ST_SUCCESS)
		{
			//ShowErrorMsg(CString("Could not get Width!"), retval);
			return false;
		}
		//ViewSize.cx = (LONG)int64Val;     // Set window size cx
		_width = int(int64Val);
		
		// Get Height from the camera
		retval = J_Camera_GetValueInt64(_hCamera, (int8_t*)NODE_NAME_HEIGHT, &int64Val);
		if (retval != J_ST_SUCCESS)
		{
			//ShowErrorMsg(CString("Could not get Height!"), retval);
			return false;
		}
		_height = int(int64Val);
		//ViewSize.cy = (LONG)int64Val;     // Set window size cy
		
		// Get pixelformat from the camera
		int64_t pixelFormat;
		retval = J_Camera_GetValueInt64(_hCamera, (int8_t*)NODE_NAME_PIXELFORMAT, &pixelFormat);
		if (retval != J_ST_SUCCESS)
		{
			//ShowErrorMsg(CString("Unable to get PixelFormat value!"), retval);
			return false;
		}

		// Calculate number of bits (not bytes) per pixel using macro
		int bpp = J_BitsPerPixel(pixelFormat);

		_sourceFrame.create(_width, _height, CV_8UC1);

		//bool bayerFormat = pixelFormat & J_GVSP_PIX_BAYRG8;
		
		// Open stream
		retval = J_Image_OpenStream(_hCamera, 0, reinterpret_cast<J_IMG_CALLBACK_OBJECT>(this),
			reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&JaiCameraNodeType::cameraStreamCallback),
			&_hStreamThread, (_width * _height * bpp)/8);
		if (retval != J_ST_SUCCESS) {
			//ShowErrorMsg(CString("Could not open stream!"), retval);
			return false;
		}

		// Start Acquision
		retval = J_Camera_ExecuteCommand(_hCamera, (int8_t*)NODE_NAME_ACQSTART);
		if (retval != J_ST_SUCCESS)
		{
			//ShowErrorMsg(CString("Could not Start Acquisition!"), retval);
			return false;
		}
		
		return true;
	}
	
	void cameraStreamCallback(J_tIMAGE_INFO* pAqImageInfo)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		
		if(_sourceFrame.step * _sourceFrame.rows == pAqImageInfo->iImageSize)
		{
			memcpy(_sourceFrame.data, pAqImageInfo->pImageBuffer, pAqImageInfo->iImageSize);
		}
		else
		{

		}
	}
	
	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		std::lock_guard<std::mutex> lock(_mutex);
		
		cv::Mat& matFrame = writer.acquireSocket(0).getImage();
		matFrame = _sourceFrame;
	
		return ExecutionStatus(EStatus::Tag);
	}
	
	void finish() override
	{
		J_STATUS_TYPE retval;
		
		// Stop Acquision
		if (_hCamera)
		{
			retval = J_Camera_ExecuteCommand(_hCamera, (int8_t*)NODE_NAME_ACQSTOP);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not Stop Acquisition!"), retval);
			}
		}

		if(_hStreamThread)
		{
			// Close stream
			retval = J_Image_CloseStream(_hStreamThread);
			if (retval != J_ST_SUCCESS)
			{
				//ShowErrorMsg(CString("Could not close Stream!"), retval);
			}
			_hStreamThread = NULL;
		}
	}
	
	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		/*
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "Video path", "filter:Video files (*.mkv *.mp4 *.avi)" },
			{ EPropertyType::Integer, "Start frame", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};
		*/

		nodeConfig.description = "Provides video frames from JAI camera";
		nodeConfig.pOutputSockets = out_config;
		/*nodeConfig.pProperties = prop_config;*/
		nodeConfig.flags = Node_HasState | Node_AutoTag;
	}
	
private:
	FACTORY_HANDLE _hFactory;
	CAM_HANDLE _hCamera;
	THRD_HANDLE _hStreamThread;
	int _width;
	int _height;
	cv::Mat _sourceFrame;
	std::mutex _mutex;
};

REGISTER_NODE("Source/JAI Camera", JaiCameraNodeType)

#endif