#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <QStringList>

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
		return true;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		Q_UNUSED(reader);

		cv::Mat& output = writer->lockSocket(0);
		if(!_capture.isOpened())
		{
			output = cv::Mat();
			return;
		}
		
		_capture.read(output);
		if(output.data)
			cv::cvtColor(output, output, CV_BGR2GRAY);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "Video path", "filter:Video files (*.mkv *.mp4 *.avi)" },
			{ EPropertyType::Integer, "Start frame", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Provides video frames from specified stream";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_State | Node_SelfTagging;
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& source = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);
		_mog(source, output, _learningRate);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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
		nodeConfig.flags = Node_State;
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		Q_UNUSED(reader);

		cv::Mat& output = writer->lockSocket(0);

		output = cv::imread(_filePath, CV_LOAD_IMAGE_GRAYSCALE);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::GaussianBlur(input, output, cv::Size(_kernelRadius*2+1,_kernelRadius*2+1), _sigma, 0);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			// TODO: In future we might use slider
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Mat xgrad;
		cv::Sobel(input, xgrad, CV_16S, 1, 0, 3);
		cv::convertScaleAbs(xgrad, xgrad);

		cv::Mat ygrad;
		cv::Sobel(input, ygrad, CV_16S, 0, 1, 3);
		cv::convertScaleAbs(ygrad, ygrad);

		cv::addWeighted(xgrad, 0.5, ygrad, 0.5, 0, output);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Canny(input, output, _threshold, _threshold*_ratio, 3);
	}


	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src1 = reader->readSocket(0);
		const cv::Mat& src2 = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if((src1.rows == 0 || src1.cols == 0) ||
			(src2.rows == 0 || src2.cols == 0))
		{
			dst = cv::Mat();
			return;
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			dst = cv::Mat();
			return;
		}

		cv::addWeighted(src1, _alpha, src2, _beta, 0, dst);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source1", "X", "" },
			{ "source2", "Y", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}

		dst = cv::abs(src);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Calculates absolute value of a given image: y = abs(x)";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class SubtractNodeType : public NodeType
{
public:
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src1 = reader->readSocket(0);
		const cv::Mat& src2 = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if((src1.rows == 0 || src1.cols == 0) ||
		   (src2.rows == 0 || src2.cols == 0))
		{
			dst = cv::Mat();
			return;
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			dst = cv::Mat();
			return;
		}

		cv::subtract(src1, src2, dst);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source1", "X", "" },
			{ "source2", "Y", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Subtracts one image from another: x - y";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class AbsoluteDifferenceNodeType : public NodeType
{
public:
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src1 = reader->readSocket(0);
		const cv::Mat& src2 = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if((src1.rows == 0 || src1.cols == 0) ||
		   (src2.rows == 0 || src2.cols == 0))
		{
			dst = cv::Mat();
			return;
		}

		if(src1.rows != src2.rows
			|| src1.cols != src2.cols)
		{
			dst = cv::Mat();
			return;
		}

		cv::absdiff(src1, src2, dst);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source1", "X", "" },
			{ "source2", "Y", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Calculates absolute values from difference of two images: |x - y|";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class NegateNodeType : public NodeType
{
public:
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(dst.type() == CV_8U)
			dst = 255 - src;
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(src.rows == 0 || src.cols == 0 || _threshold < 0 || _threshold > 255)
		{
			dst = cv::Mat();
			return;
		}

		int type = _inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
		cv::threshold(src, dst, (double) _threshold, 255, type);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		int ddepth = _scaleAbs ? CV_16S : -1;

		cv::Mat kernel(3, 3, CV_64FC1, _coeffs.v);
		cv::filter2D(src, dst, ddepth, kernel);

		if(_scaleAbs)
			cv::convertScaleAbs(dst, dst);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		cv::Mat kernel = cvu::predefinedConvolutionKernel(_filterType);
		bool weight0 = cv::sum(kernel) == cv::Scalar(0);
		int ddepth = weight0 ? CV_16S : -1;

		cv::filter2D(src, dst, ddepth, kernel);

		if(weight0)
			cv::convertScaleAbs(dst, dst);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		cv::Mat& kernel = writer->lockSocket(0);

		if(_xradius == 0 || _yradius == 0)
		{
			kernel = cv::Mat();
			return;
		}

		kernel = cvu::standardStructuringElement(_xradius, _yradius, _se, _rotation);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ "structuringElement", "Structuring element", "" },
			{ "", "", "" }
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		const cv::Mat& src = reader->readSocket(0);
		const cv::Mat& se = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if(se.cols == 0 || se.rows == 0 || src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}
		
		cv::morphologyEx(src, dst, _op, se);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "source", "Structuring element", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
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


REGISTER_NODE("Mixture of Gaussians", MixtureOfGaussianNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)

REGISTER_NODE("Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Predefined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Morphology op.", MorphologyNodeType)
REGISTER_NODE("Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Binarization", BinarizationNodeType)

REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Absolute diff.", AbsoluteDifferenceNodeType)
REGISTER_NODE("Subtract", SubtractNodeType)
REGISTER_NODE("Absolute", AbsoluteNodeType)
REGISTER_NODE("Add", AddNodeType)

REGISTER_NODE("Structuring element", StructuringElementNodeType)
REGISTER_NODE("Video from file", VideoFromFileNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)