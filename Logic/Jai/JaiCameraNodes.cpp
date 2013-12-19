#if defined(HAVE_JAI)

#include "NodeType.h"
#include "NodeFactory.h"

#include "JaiNodeModule.h"
#include "JaiException.h"

#include <opencv2/core/core.hpp>
#include <mutex>

class JaiCameraNodeType : public NodeType
{
public:
	JaiCameraNodeType()
		: _hCamera(nullptr)
		, _hStreamThread(nullptr)
		, _width(0)
		, _height(0)
	{
	}

	bool init(const std::shared_ptr<NodeModule>& nodeModule) override
	{
		try
		{
			_jaiModule = std::dynamic_pointer_cast<JaiNodeModule>(nodeModule);
			if(!_jaiModule)
				return false;

			_hCamera = _jaiModule->openCamera(0);
			return _hCamera != nullptr;
		}
		catch(JaiException&)
		{
			return false;
		}
	}	
	
	bool restart() override
	{
		_width = int(queryNodeValue<int64_t>(_hCamera, NODE_NAME_WIDTH));
		_height = int(queryNodeValue<int64_t>(_hCamera, NODE_NAME_HEIGHT));
		int64_t pixelFormat = queryNodeValue<int64_t>(_hCamera, NODE_NAME_PIXELFORMAT);
		int bpp = J_BitsPerPixel(pixelFormat);

		switch(bpp)
		{
		case 8:
			_sourceFrame.create(_height, _width, CV_8UC1);
			break;
		case 16:
			_sourceFrame.create(_height, _width, CV_16UC1);
			break;
		}
		
		// Open stream
		J_STATUS_TYPE error;
		if ((error = J_Image_OpenStream(_hCamera, 0, reinterpret_cast<J_IMG_CALLBACK_OBJECT>(this),
			reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&JaiCameraNodeType::cameraStreamCallback),
			&_hStreamThread, (_width * _height * bpp)/8)) != J_ST_SUCCESS)
		{
			throw JaiException(error, "Stream couldn't be opened");
		}

		// Start Acquision
		if ((error = J_Camera_ExecuteCommand(_hCamera, 
			(int8_t*)NODE_NAME_ACQSTART)) != J_ST_SUCCESS)
		{
			throw JaiException(error, "Acquisition couldn't be started");
		}
		
		return true;
	}
	
	void cameraStreamCallback(J_tIMAGE_INFO* pAqImageInfo)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		
		if(_sourceFrame.step * _sourceFrame.rows == pAqImageInfo->iImageSize)
			memcpy(_sourceFrame.data, pAqImageInfo->pImageBuffer, pAqImageInfo->iImageSize);
	}
	
	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& matFrame = writer.acquireSocket(0).getImage();
		{
			std::lock_guard<std::mutex> lock(_mutex);
			matFrame = _sourceFrame;
		}
		return ExecutionStatus(EStatus::Tag);
	}
	
	void finish() override
	{
		J_STATUS_TYPE error;
		
		// Stop Acquision
		if (_hCamera)
		{
			if ((error = J_Camera_ExecuteCommand(_hCamera,
				(int8_t*)NODE_NAME_ACQSTOP)) != J_ST_SUCCESS)
			{}// Doesn't make any sense to do anything here
		}

		if(_hStreamThread)
		{
			// Close stream
			if ((error = J_Image_CloseStream(_hStreamThread)) != J_ST_SUCCESS)
			{}// Doesn't make any sense to do anything here
			_hStreamThread = nullptr;
		}
	}
	
	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		/// TODO: CameraID
		/// - Rest of properties should be settable via special module menu
		/*
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "Video path", "filter:Video files (*.mkv *.mp4 *.avi)" },
			{ EPropertyType::Integer, "Start frame", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};
		*/

		nodeConfig.description = "Provides video frames from JAI camera";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.module = "jai";
		//nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState | Node_AutoTag;
	}
	
private:
	CAM_HANDLE _hCamera;
	THRD_HANDLE _hStreamThread;
	int _width;
	int _height;
	cv::Mat _sourceFrame;
	std::mutex _mutex;

	std::shared_ptr<JaiNodeModule> _jaiModule;
};

REGISTER_NODE("Source/JAI Camera", JaiCameraNodeType)

#endif