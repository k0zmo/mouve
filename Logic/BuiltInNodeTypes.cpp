#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <chrono>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

class VideoFromFileNodeType : public NodeType
{
public:
	VideoFromFileNodeType()
		: _videoPath("video-4.mkv")
		, _startFrame(0)
		, _frameInterval(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_VideoPath:
			_videoPath = newValue.toString().toStdString();
			return true;
		case ID_StartFrame:
			_startFrame = newValue.toUInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_VideoPath: return QString::fromStdString(_videoPath);
		case ID_StartFrame: return _startFrame;
		}

		return QVariant();
	}

	bool initialize() override
	{
		// This isn't really necessary as subsequent open()
		// should call it automatically
		if(_capture.isOpened())
			_capture.release();

		_capture.open(_videoPath);
		if(!_capture.isOpened())
			return false;

		if(_startFrame > 0)
			_capture.set(CV_CAP_PROP_POS_FRAMES, _startFrame);

		double fps = _capture.get(CV_CAP_PROP_FPS);

		_frameInterval = (fps != 0)
			? unsigned(ceil(1000.0 / fps))
			: 0;
		_timeStamp = std::chrono::high_resolution_clock::time_point();

		return true;
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		if(!_capture.isOpened())
			return ExecutionStatus(EStatus::Ok);

		// We need this so we won't replace previous good frame with current bad (for instance - end of video)
		cv::Mat buffer; 

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
			}

			_capture.read(buffer);

			_timeStamp = std::chrono::high_resolution_clock::now();
		}

		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(buffer.data)
		{
			cv::cvtColor(buffer, output, CV_BGR2GRAY);
			return ExecutionStatus(EStatus::Tag);
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
			{ EPropertyType::Filepath, "Video path", "filter:Video files (*.mkv *.mp4 *.avi)" },
			{ EPropertyType::Integer, "Start frame", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Provides video frames from specified stream";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState | Node_AutoTag;
	}

private:
	enum EPropertyID
	{
		ID_VideoPath,
		ID_StartFrame
	};

	std::string _videoPath;
	cv::VideoCapture _capture;
	unsigned _startFrame;
	unsigned _frameInterval;
	std::chrono::high_resolution_clock::time_point _timeStamp;
};

class MixtureOfGaussianNodeType : public NodeType
{
public:
	MixtureOfGaussianNodeType()
		: _history(200)
		, _nmixtures(5)
		, _backgroundRatio(0.7)
		, _learningRate(-1)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_History:
			_history = newValue.toInt();
			return true;
		case ID_NMixtures:
			_nmixtures = newValue.toInt();
			return true;
		case ID_BackgroundRatio:
			_backgroundRatio = newValue.toDouble();
			return true;
		case ID_LearningRate:
			_learningRate = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_History: return _history;
		case ID_NMixtures: return _nmixtures;
		case ID_BackgroundRatio: return _backgroundRatio;
		case ID_LearningRate: return _learningRate;
		}

		return QVariant();
	}

	bool initialize() override
	{
		_mog = cv::BackgroundSubtractorMOG(_history, _nmixtures, _backgroundRatio);
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& source = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(!source.data)
			return ExecutionStatus(EStatus::Ok);

		_mog(source, output, _learningRate);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "History frames", "min:1, max:500" },
			{ EPropertyType::Integer, "Number of mixtures",  "min:1, max:9" },
			{ EPropertyType::Double, "Background ratio", "min:0.01, max:0.99, step:0.01" },
			{ EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Gaussian Mixture-based Background/Foreground Segmentation Algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID
	{
		ID_History,
		ID_NMixtures,
		ID_BackgroundRatio,
		ID_LearningRate
	};

	cv::BackgroundSubtractorMOG _mog;
	int _history;
	int _nmixtures;
	double _backgroundRatio;
	double _learningRate;
};

class ImageFromFileNodeType : public NodeType
{
public: 
	ImageFromFileNodeType()
		: _filePath("lena.jpg")
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		if(propId == 0)
		{
			_filePath = newValue.toString().toStdString();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		if(propId == 0)
			return QString::fromStdString(_filePath);

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& output = writer.acquireSocket(0).getImage();

		output = cv::imread(_filePath, CV_LOAD_IMAGE_GRAYSCALE);

		if(output.data && output.rows > 0 && output.cols > 0)
			return ExecutionStatus(EStatus::Ok);
		else
			return ExecutionStatus(EStatus::Error, "File not found");
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

		nodeConfig.description = "Loads image from a given location";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	std::string _filePath;
};

class GaussianBlurNodeType : public NodeType
{
public:
	GaussianBlurNodeType()
		: _kernelRadius(2)
		, _sigma(10.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_KernelSize:
			_kernelRadius = newValue.toInt();
			return true;
		case ID_Sigma:
			_sigma = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_KernelSize: return _kernelRadius;
		case ID_Sigma: return _sigma;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(!input.data)
			return ExecutionStatus(EStatus::Ok);

		cv::GaussianBlur(input, output, cv::Size(_kernelRadius*2+1,_kernelRadius*2+1), _sigma, 0);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			/// TODO: In future we might use slider
			{ EPropertyType::Integer, "Kernel radius", "min:1, max:20, step:1" },
			{ EPropertyType::Double, "Sigma", "min:0.0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs Gaussian blur on input image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_KernelSize,
		ID_Sigma
	};

	int _kernelRadius;
	double _sigma;
};

class SobelFilterNodeType : public NodeType
{
public:
	SobelFilterNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::Mat xgrad;
		cv::Sobel(input, xgrad, CV_16S, 1, 0, 3);
		cv::convertScaleAbs(xgrad, xgrad);

		cv::Mat ygrad;
		cv::Sobel(input, ygrad, CV_16S, 0, 1, 3);
		cv::convertScaleAbs(ygrad, ygrad);

		cv::addWeighted(xgrad, 0.5, ygrad, 0.5, 0, output);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Calculates the first image derivatives using Sobel operator";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	CannyEdgeDetectorNodeType()
		: _threshold(10)
		, _ratio(3)
	{
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_Ratio: return _ratio;
		}

		return QVariant();
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toDouble();
			return true;
		case ID_Ratio:
			_ratio = newValue.toDouble();
			return true;
		}

		return false;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);;

		cv::Canny(input, output, _threshold, _threshold*_ratio, 3);

		return ExecutionStatus(EStatus::Ok);
	}


	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Threshold", "min:0.0, max:100.0, decimals:3" },
			{ EPropertyType::Double, "Ratio", "min:0.0, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Detects edges in input image using Canny detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Ratio,
	};

	double _threshold;
	double _ratio;
};

class AddNodeType : public NodeType
{
public:
	AddNodeType()
		: _alpha(0.5)
		, _beta(0.5)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Alpha:
			_alpha = newValue.toDouble();
			return true;
		case ID_Beta:
			_beta = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Alpha: return _alpha;
		case ID_Beta: return _beta;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src1 = reader.readSocket(0).getImage();
		const cv::Mat& src2 = reader.readSocket(1).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if((src1.rows == 0 || src1.cols == 0) ||
			(src2.rows == 0 || src2.cols == 0))
		{
			return ExecutionStatus(EStatus::Ok);
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			return ExecutionStatus(EStatus::Error, "Inputs with different sizes");
		}

		cv::addWeighted(src1, _alpha, src2, _beta, 0, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source1", "X", "" },
			{ ENodeFlowDataType::Image, "source2", "Y", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Alpha", "min:0.0, max:1.0, decimals:3, step:0.1" },
			{ EPropertyType::Double, "Beta",  "min:0.0, max:1.0, decimals:3, step:0.1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Adds one image to another with specified weights: alpha * x + beta * y";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Alpha,
		ID_Beta
	};

	double _alpha;
	double _beta;
};

class AbsoluteNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		dst = cv::abs(src);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Calculates absolute value of a given image: y = abs(x)";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class SubtractNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src1 = reader.readSocket(0).getImage();
		const cv::Mat& src2 = reader.readSocket(1).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if((src1.rows == 0 || src1.cols == 0) ||
		   (src2.rows == 0 || src2.cols == 0))
		{
			return ExecutionStatus(EStatus::Ok);
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			return ExecutionStatus(EStatus::Error, "Inputs with different sizes");
		}

		cv::subtract(src1, src2, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source1", "X", "" },
			{ ENodeFlowDataType::Image, "source2", "Y", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Subtracts one image from another: x - y";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class AbsoluteDifferenceNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src1 = reader.readSocket(0).getImage();
		const cv::Mat& src2 = reader.readSocket(1).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if((src1.rows == 0 || src1.cols == 0) ||
		   (src2.rows == 0 || src2.cols == 0))
		{
			return ExecutionStatus(EStatus::Ok);;
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			return ExecutionStatus(EStatus::Error, "Inputs with different sizes");
		}

		cv::absdiff(src1, src2, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source1", "X", "" },
			{ ENodeFlowDataType::Image, "source2", "Y", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Calculates absolute values from difference of two images: |x - y|";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class NegateNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(dst.type() == CV_8U)
			dst = 255 - src;

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Negates image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class BinarizationNodeType : public NodeType
{
public:
	BinarizationNodeType()
		: _threshold(128)
		, _inv(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toInt();
			return true;
		case ID_Invert:
			_inv = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_Invert: return _inv;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		if(_threshold < 0 || _threshold > 255)
			return ExecutionStatus(EStatus::Error, "Bad threshold value");

		int type = _inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
		cv::threshold(src, dst, (double) _threshold, 255, type);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Threshold", "min:0, max:255" },
			{ EPropertyType::Boolean, "Inverted", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 depending on threshold value";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Invert
	};

	int _threshold;
	bool _inv;
};

class CustomConvolutionNodeType : public NodeType
{
public:
	CustomConvolutionNodeType()
		: _coeffs(1)
		, _scaleAbs(true)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Coefficients:
			_coeffs = newValue.value<Matrix3x3>();
			return true;
		case ID_ScaleAbs:
			_scaleAbs = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Coefficients: return QVariant::fromValue<Matrix3x3>(_coeffs);
		case ID_ScaleAbs: return _scaleAbs;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		int ddepth = _scaleAbs ? CV_16S : -1;

		cv::Mat kernel(3, 3, CV_64FC1, _coeffs.v);
		cv::filter2D(src, dst, ddepth, kernel);

		if(_scaleAbs)
			cv::convertScaleAbs(dst, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Matrix, "Coefficients", "" },
			{ EPropertyType::Boolean, "Scale absolute", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs image convolution";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Coefficients,
		ID_ScaleAbs
	};

	Matrix3x3 _coeffs;
	bool _scaleAbs;
};

#include "CV.h"

class PredefinedConvolutionNodeType : public NodeType
{
public:
	PredefinedConvolutionNodeType()
		: _filterType(cvu::EPredefinedConvolutionType::NoOperation)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case 0:
			_filterType = cvu::EPredefinedConvolutionType(newValue.toInt());
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case 0: return int(_filterType);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		cv::Mat kernel = cvu::predefinedConvolutionKernel(_filterType);
		bool weight0 = cv::sum(kernel) == cv::Scalar(0);
		int ddepth = weight0 ? CV_16S : -1;

		cv::filter2D(src, dst, ddepth, kernel);

		if(weight0)
			cv::convertScaleAbs(dst, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Kernel",
				"item: No operation, item: Average, item: Gaussian, item: Mean removal,"
				"item: Robert's cross 45, item: Robert's cross 135, item: Laplacian,"
				"item: Prewitt horizontal, item: Prewitt vertical, "
				"item: Sobel horizontal, item: Sobel vertical"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs image convolution using one of predefined convolution kernel";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	cvu::EPredefinedConvolutionType _filterType;
};

class StructuringElementNodeType : public NodeType
{
public:

	StructuringElementNodeType()
		: _se(cvu::EStructuringElementType::Ellipse)
		, _xradius(1)
		, _yradius(1)
		, _rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_StructuringElementType:
			_se = cvu::EStructuringElementType(newValue.toInt());
			return true;
		case ID_XRadius:
			_xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			_yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			_rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_StructuringElementType: return int(_se);
		case ID_XRadius: return _xradius;
		case ID_YRadius: return _yradius;
		case ID_Rotation: return _rotation;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& kernel = writer.acquireSocket(0).getImage();

		if(_xradius == 0 || _yradius == 0)
			return ExecutionStatus(EStatus::Ok);

		kernel = cvu::standardStructuringElement(_xradius, _yradius, _se, _rotation);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "structuringElement", "Structuring element", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "SE shape", "item: Rectangle, item: Ellipse, item: Cross" },
			{ EPropertyType::Integer, "Horizontal radius", "min:1, max:50" },
			{ EPropertyType::Integer, "Vertical radius", "min:1, max:50" },
			{ EPropertyType::Integer, "Rotation", "min:0, max:359, wrap:true" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Generates a structuring element for a morphology "
			"operation with a given parameters describing its shape, size and rotation";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_StructuringElementType,
		ID_XRadius,
		ID_YRadius,
		ID_Rotation
	};

	cvu::EStructuringElementType _se;
	int _xradius;
	int _yradius;
	int _rotation;
};

class MorphologyNodeType : public NodeType
{
public:
	MorphologyNodeType()
		: _op(Erode)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			_op = EMorphologyOperation(newValue.toInt());
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Operation: return int(_op);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		const cv::Mat& se = reader.readSocket(1).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(se.cols == 0 || se.rows == 0 || src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);
		
		cv::morphologyEx(src, dst, _op, se);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Image, "source", "Structuring element", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
				"item: Erode, item: Dilate, item: Open, item: Close,"
				"item: Gradient, item: Top Hat, item: Black Hat"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs a morphology operation on a given image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EMorphologyOperation
	{
		Erode    = cv::MORPH_ERODE,
		Dilate   = cv::MORPH_DILATE,
		Open     = cv::MORPH_OPEN,
		Close    = cv::MORPH_CLOSE,
		Gradient = cv::MORPH_GRADIENT,
		TopHat   = cv::MORPH_TOPHAT,
		BlackHat = cv::MORPH_BLACKHAT
	};

	enum EPropertyID
	{
		ID_Operation
	};

	EMorphologyOperation _op;
};

class EqualizeHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::equalizeHist(src, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DownsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::pyrDown(src, dst, cv::Size(src.cols/2, src.rows/2));

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class UpsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::pyrUp(src, dst, cv::Size(src.cols*2, src.rows*2));

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

// _________________________________________________________________________________________________

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#ifdef _DEBUG
#  pragma comment(lib, "opencv_features2d243d.lib")
#  pragma comment(lib, "opencv_nonfree243d.lib")
#  pragma comment(lib, "opencv_flann243d.lib")
#  pragma comment(lib, "opencv_calib3d243d.lib")
#else
#  pragma comment(lib, "opencv_features2d243.lib")
#  pragma comment(lib, "opencv_nonfree243.lib")
#  pragma comment(lib, "opencv_flann243.lib")
#  pragma comment(lib, "opencv_calib3d243.lib")
#endif

class SurfFeatureDetectorNodeType : public NodeType
{
public:
	SurfFeatureDetectorNodeType()
		: _hessianThreshold(400.0)
		, _nOctaves(4)
		, _nOctaveLayers(2)
		, _extended(true)
		, _upright(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_HessianThreshold:
			_hessianThreshold = newValue.toDouble();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case ID_NumOctaveLayers:
			_nOctaveLayers = newValue.toInt();
			return true;
		case ID_Extended:
			_extended = newValue.toBool();
			return true;
		case ID_Upright:
			_upright = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_HessianThreshold: return _hessianThreshold;
		case ID_NumOctaves: return _nOctaves;
		case ID_NumOctaveLayers: return _nOctaveLayers;
		case ID_Extended: return _extended;
		case ID_Upright: return _upright;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::KeyPoints& keypoints = writer.acquireSocket(0).getKeypoints();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves,
			_nOctaveLayers, _extended, _upright);
		detector.detect(src, keypoints);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "" },
			{ EPropertyType::Integer, "Number of octave layers", "" },
			{ EPropertyType::Boolean, "Extended", "" },
			{ EPropertyType::Boolean, "Upright", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumOctaveLayers,
		ID_Extended,
		ID_Upright
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nOctaveLayers;
	bool _extended;
	bool _upright;
};

class SurfFeatureExtractorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints = reader.readSocket(1).getKeypoints();

		if(keypoints.empty() || imageSrc.cols == 0 || imageSrc.rows == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::KeyPoints& outKeypoints = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();

		outKeypoints = keypoints;

		cv::SurfDescriptorExtractor extractor;
		extractor.compute(imageSrc, outKeypoints, outDescriptors);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class NearestNeighborMatcherNodeType : public NodeType
{
public:
	NearestNeighborMatcherNodeType()
		: _threshold(2.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& queryDescriptors = reader.readSocket(0).getArray();
		const cv::Mat& trainDescriptors = reader.readSocket(1).getArray();

		if(!queryDescriptors.data || !trainDescriptors.data)
			return ExecutionStatus(EStatus::Ok);

		cv::DMatches& matches = writer.acquireSocket(0).getMatches();

		if(qFuzzyCompare(0.0, _threshold))
		{

			cv::FlannBasedMatcher matcher;
			matcher.match(queryDescriptors, trainDescriptors, matches);
		}
		else
		{
			cv::DMatches tmpMatches;
			cv::FlannBasedMatcher matcher;
			matcher.match(queryDescriptors, trainDescriptors, tmpMatches);

			double max_dist = -std::numeric_limits<double>::max();
			double min_dist = +std::numeric_limits<double>::max();

			for(int i = 0; i < queryDescriptors.rows; ++i)
			{
				double dist = tmpMatches[i].distance;
				min_dist = qMin(min_dist, dist);
				max_dist = qMax(max_dist, dist);
			}

			matches.clear();

			double th = _threshold * min_dist;

			for(int i = 0; i < queryDescriptors.rows; ++i)
			{
				if(tmpMatches[i].distance < th)
					matches.push_back(tmpMatches[i]);
			}
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Array, "descriptors1", "1st descriptors", "" },
			{ ENodeFlowDataType::Array, "descriptors2", "2nd descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Threshold", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold
	};

	double _threshold;
};

class EstimateHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::KeyPoints& keypoints1 = reader.readSocket(0).getKeypoints();
		const cv::KeyPoints& keypoints2 = reader.readSocket(1).getKeypoints();
		const cv::DMatches& matches = reader.readSocket(2).getMatches();
		cv::Mat& H = writer.acquireSocket(0).getArray();

		if(keypoints1.empty() || keypoints2.empty() || matches.empty())
			return ExecutionStatus(EStatus::Ok);

		//-- Localize the object
		std::vector<cv::Point2f> obj;
		std::vector<cv::Point2f> scene;

		for( int i = 0; i < matches.size(); i++ )
		{
			obj.push_back(keypoints1[matches[i].queryIdx].pt);
			scene.push_back(keypoints2[matches[i].trainIdx].pt);
		}

		H = cv::findHomography(obj, scene, cv::RANSAC);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints1", "1st Keypoints", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints1", "2nd Keypoints", "" },
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "output", "Homography", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DrawKeypointsNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints = reader.readSocket(1).getKeypoints();
		cv::Mat& imageDst = writer.acquireSocket(0).getImage();

		if(keypoints.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::drawKeypoints(imageSrc, keypoints, imageDst, 
			cv::Scalar::all(1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		cv::cvtColor(imageDst, imageDst, CV_BGR2GRAY);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DrawMatchesNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& image1 = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints1 = reader.readSocket(1).getKeypoints();
		const cv::Mat& image2 = reader.readSocket(2).getImage();
		const cv::KeyPoints& keypoints2 = reader.readSocket(3).getKeypoints();
		const cv::DMatches& matches = reader.readSocket(4).getMatches();

		cv::Mat& imageMatches = writer.acquireSocket(0).getImage();

		if(keypoints1.empty() || keypoints2.empty() || !image1.data || !image2.data || matches.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::drawMatches(image1, keypoints1, image2, keypoints2,
			matches, imageMatches, cv::Scalar::all(1), cv::Scalar::all(1),
			std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
		cv::cvtColor(imageMatches, imageMatches, CV_BGR2GRAY);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image1", "1st Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints1", "1st Keypoints", "" },
			{ ENodeFlowDataType::Image, "image2", "2nd Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints2", "2nd Keypoints", "" },
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DrawHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& img_scene = reader.readSocket(0).getImage();
		const cv::Mat& img_object = reader.readSocket(1).getImage();
		const cv::Mat& homography = reader.readSocket(2).getArray();

		cv::Mat& img_matches = writer.acquireSocket(0).getImage();

		if(!img_scene.data || !img_object.data)
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Point2f> obj_corners(4);
		obj_corners[0] = cvPoint(0,0);
		obj_corners[1] = cvPoint(img_object.cols, 0);
		obj_corners[2] = cvPoint(img_object.cols, img_object.rows);
		obj_corners[3] = cvPoint(0, img_object.rows);

		std::vector<cv::Point2f> scene_corners(4);

		cv::perspectiveTransform(obj_corners, scene_corners, homography);

		img_matches = img_scene.clone();

		cv::line(img_matches, scene_corners[0], scene_corners[1], cv::Scalar(0, 0, 0), 4);
		cv::line(img_matches, scene_corners[1], scene_corners[2], cv::Scalar(0, 0, 0), 4);
		cv::line(img_matches, scene_corners[2], scene_corners[3], cv::Scalar(0, 0, 0), 4);
		cv::line(img_matches, scene_corners[3], scene_corners[0], cv::Scalar(0, 0, 0), 4);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Base image", "" },
			{ ENodeFlowDataType::Image, "source", "Object image", "" },
			{ ENodeFlowDataType::Array, "source", "Homography", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Draw homography", DrawHomographyNodeType)
REGISTER_NODE("Draw matches", DrawMatchesNodeType);
REGISTER_NODE("Draw keypoints", DrawKeypointsNodeType);
REGISTER_NODE("Estimate homography", EstimateHomographyNodeType)
REGISTER_NODE("NN Matcher", NearestNeighborMatcherNodeType);
REGISTER_NODE("SURF Extractor", SurfFeatureExtractorNodeType)
REGISTER_NODE("SURF Detector", SurfFeatureDetectorNodeType);

REGISTER_NODE("Mixture of Gaussians", MixtureOfGaussianNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)

REGISTER_NODE("Upsample", UpsampleNodeType)
REGISTER_NODE("Downsample", DownsampleNodeType)
REGISTER_NODE("Equalize hist.", EqualizeHistogramNodeType)

REGISTER_NODE("Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Predefined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Morphology op.", MorphologyNodeType)
REGISTER_NODE("Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Binarization", BinarizationNodeType)

REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Absolute diff.", AbsoluteDifferenceNodeType)
//REGISTER_NODE("Subtract", SubtractNodeType)
//REGISTER_NODE("Absolute", AbsoluteNodeType)
REGISTER_NODE("Add", AddNodeType)

REGISTER_NODE("Structuring element", StructuringElementNodeType)
REGISTER_NODE("Video from file", VideoFromFileNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)

