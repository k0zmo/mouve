#include "NodeType.h"
#include "NodeFactory.h"
#include "Kommon/HighResolutionClock.h"

#include <chrono>
#include <thread>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class VideoFromFileNodeType : public NodeType
{
public:
	VideoFromFileNodeType()
#ifdef QT_DEBUG
		: _videoPath("video-4.mkv")
#else
		: _videoPath("")
#endif
		, _startFrame(0)
		, _endFrame(0)
		, _frameInterval(0)
		, _ignoreFps(false)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::VideoPath:
			_videoPath = newValue.toFilepath();
			return true;
		case pid::StartFrame:
			_startFrame = newValue.toInt();
			return true;
		case pid::EndFrame:
			_endFrame = newValue.toInt();
			return true;
		case pid::IgnoreFps:
			_ignoreFps = newValue.toBool();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::VideoPath: return _videoPath;
		case pid::StartFrame: return int(_startFrame);
		case pid::EndFrame: return int(_endFrame);
		case pid::IgnoreFps: return _ignoreFps;
		}

		return NodeProperty();
	}

	bool restart() override
	{
		// This isn't really necessary as subsequent open()
		// should call it automatically
		if(_capture.isOpened())
			_capture.release();

		_capture.open(_videoPath.data());
		if(!_capture.isOpened())
			return false;

		if(_startFrame > 0)
			_capture.set(CV_CAP_PROP_POS_FRAMES, _startFrame);

		double fps = _ignoreFps ? 0 : _capture.get(CV_CAP_PROP_FPS);

		_frameInterval = (fps != 0)
			? unsigned(ceil(1000.0 / fps))
			: 0;
		_timeStamp = std::chrono::high_resolution_clock::time_point();
		_maxFrames = static_cast<unsigned>(_capture.get(CV_CAP_PROP_FRAME_COUNT));
		_currentFrame = _startFrame > 0 ? _startFrame : 0;

		return true;
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		double start = _clock.currentTimeInSeconds();

		if(!_capture.isOpened())
			return ExecutionStatus(EStatus::Ok);

		// We need this so we won't replace previous good frame with current bad (for instance - end of video)
		cv::Mat buffer; 
		++_currentFrame;

		if(_endFrame > 0 && _currentFrame >= _endFrame)
		{
			// No more data (for us)
			return ExecutionStatus(EStatus::Ok, formatMessage("Frame image width: %d\nFrame image height: %d\nFrame size in kbytes: %d\nFrame: %d/%d",
				buffer.cols, buffer.rows, buffer.cols * buffer.rows * sizeof(uchar) * buffer.channels() / 1024, _currentFrame - 1, _maxFrames));
		}

		if(_frameInterval == 0)
		{
			_capture.read(buffer);
		}
		else
		{
			using namespace std::chrono;

			// This should keep FPS in sync
			high_resolution_clock::time_point s = high_resolution_clock::now();
			milliseconds dura = duration_cast<milliseconds>(s - _timeStamp);
			if(dura.count() < _frameInterval)
			{
				milliseconds waitDuration = milliseconds(_frameInterval) - dura;
				std::this_thread::sleep_for(waitDuration);
				// New start time
				start = _clock.currentTimeInSeconds();
			}

			_capture.read(buffer);

			_timeStamp = std::chrono::high_resolution_clock::now();
		}

		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(!buffer.empty())
		{
			cv::cvtColor(buffer, output, CV_BGR2GRAY);
			double stop = _clock.currentTimeInSeconds();
			double elapsed = (stop - start) * 1e3;

			return ExecutionStatus(EStatus::Tag, elapsed,
				formatMessage("Frame image width: %d\nFrame image height: %d\nFrame size in kbytes: %d\nFrame: %d/%d",
					output.cols, output.rows, output.cols * output.rows * sizeof(uchar) * output.channels() / 1024, _currentFrame, _maxFrames));
		}
		else
		{
			// No more data 
			return ExecutionStatus(EStatus::Ok);
		}
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "Video path", "filter:Video files (*.mkv *.mp4 *.avi *.flv)" },
			{ EPropertyType::Integer, "Start frame", "min:0" },
			{ EPropertyType::Integer, "End frame", "min:0" },
			{ EPropertyType::Boolean, "Ignore FPS", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Provides video frames from specified stream.";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = ENodeConfig::HasState | ENodeConfig::AutoTag | ENodeConfig::OverridesTimeComputation;
	}

private:
	enum class pid
	{
		VideoPath,
		StartFrame,
		EndFrame,
		IgnoreFps,
	};

	Filepath _videoPath;
	cv::VideoCapture _capture;
	unsigned _startFrame;
	unsigned _endFrame;
	unsigned _frameInterval;
	unsigned _currentFrame;
	unsigned _maxFrames;
	std::chrono::high_resolution_clock::time_point _timeStamp;
	static HighResolutionClock _clock;
	bool _ignoreFps;
};
HighResolutionClock VideoFromFileNodeType::_clock;

class ImageFromFileNodeType : public NodeType
{
public: 
	ImageFromFileNodeType()
#ifdef QT_DEBUG
		: _filePath("lena.jpg")
#else
		: _filePath("")
#endif
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		if(propId == 0)
		{
			_filePath = newValue.toFilepath();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		if(propId == 0)
			return _filePath;

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& output = writer.acquireSocket(0).getImage();

		output = cv::imread(_filePath.data(), CV_LOAD_IMAGE_GRAYSCALE);

		if(output.empty())
			return ExecutionStatus(EStatus::Error, "File not found");
		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Image image width: %d\nImage image height: %d\nImage size in kbytes: %d",
				output.cols, output.rows, output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "File path", "filter:"
			"Popular image formats (*.bmp *.jpeg *.jpg *.png *.tiff);;"
			"Windows bitmaps (*.bmp *.dib);;"
			"JPEG files (*.jpeg *.jpg *.jpe);;"
			"JPEG 2000 files (*.jp2);;"
			"Portable Network Graphics (*.png);;"
			"Portable image format (*.pbm *.pgm *.ppm);;"
			"Sun rasters (*.sr *.ras);;"
			"TIFF files (*.tiff *.tif);;"
			"All files (*.*)" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Loads image from a given location.";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	Filepath _filePath;
};

class ImageFromFileStreamNodeType : public ImageFromFileNodeType
{
public: 
	bool restart() override
	{ 
		_img = cv::imread(_filePath.data(), CV_LOAD_IMAGE_GRAYSCALE);

		return !_img.empty();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& output = writer.acquireSocket(0).getImage();

		output = _img;

		return ExecutionStatus(EStatus::Tag, 
			formatMessage("Image image width: %d\nImage image height: %d\nImage size in kbytes: %d",
			output.cols, output.rows, output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		ImageFromFileNodeType::configuration(nodeConfig);
		nodeConfig.flags = ENodeConfig::AutoTag | ENodeConfig::HasState;
	}

private:
	cv::Mat _img;
};

class CameraCaptureNodeType : public NodeType
{
public:
	CameraCaptureNodeType()
		: _deviceId(0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::DeviceId:
			_deviceId = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::DeviceId: return _deviceId;
		}

		return NodeProperty();
	}

	bool restart() override
	{
		_capture.open(_deviceId);
		if(!_capture.isOpened())
			return false;
		return true;
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& output = writer.acquireSocket(0).getImage();

		/// TODO: Could be moved to background thread (just like JAI)
		cv::Mat tmp;
		_capture >> tmp;

		if(tmp.empty())
			return ExecutionStatus(EStatus::Ok);
		if(tmp.channels() > 1)
			cvtColor(tmp, output, CV_BGR2GRAY);

		return ExecutionStatus(EStatus::Tag, 
			formatMessage("Frame image width: %d\nFrame image height: %d\nFrame size in kbytes: %d",
				output.cols, output.rows, output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
	}

	void finish() override
	{
		if(_capture.isOpened())
			_capture.release();
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Device Id", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Provides video frames from specified camera device.";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = ENodeConfig::HasState | ENodeConfig::AutoTag;
	}

private:
	enum class pid
	{
		DeviceId
	};

	int _deviceId;
	cv::VideoCapture _capture;
};

REGISTER_NODE("Sources/Image from file stream", ImageFromFileStreamNodeType)
REGISTER_NODE("Sources/Camera capture", CameraCaptureNodeType)
REGISTER_NODE("Sources/Video from file", VideoFromFileNodeType)
REGISTER_NODE("Sources/Image from file", ImageFromFileNodeType)
