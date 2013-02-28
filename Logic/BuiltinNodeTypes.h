#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QDebug>

class VideoFromFileNodeType : public NodeType
{
public:
	VideoFromFileNodeType()
		: _videoPath("video-1.mkv")
		, _startFrame(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case VideoPath:
			_videoPath = newValue.toString().toStdString();
			return true;
		case StartFrame:
			_startFrame = newValue.toUInt();
			return true;
		}

		return false;
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
		qDebug() << "Executing 'Video from file' node";

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
			{ EPropertyType::Filepath, "Video path", QVariant(QString::fromStdString(_videoPath)), "" },
			{ EPropertyType::Integer, "Start frame", QVariant(_startFrame), "min=0" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Provides video frames from specified stream";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_State | Node_SelfTagging;
	}

private:
	enum EPropertyID
	{
		VideoPath,
		StartFrame
	};

	std::string _videoPath;
	cv::VideoCapture _capture;
	unsigned _startFrame;
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		Q_UNUSED(reader);

		qDebug() << "Executing 'Image from file' node";		

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
			{ EPropertyType::Filepath, "File path", QVariant(QString::fromStdString(_filePath)), "" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
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
		: kernelRadius(2)
		, sigma(10.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_KernelSize:
			kernelRadius = newValue.toInt();
			return true;
		case ID_Sigma:
			sigma = newValue.toDouble();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Gaussian blur' node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::GaussianBlur(input, output, cv::Size(kernelRadius*2+1,kernelRadius*2+1), sigma, 0);
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
			{ EPropertyType::Integer, "Kernel radius", QVariant(kernelRadius), "min=1;max=20;step=1" },
			{ EPropertyType::Double, "Sigma", QVariant(sigma), "min=0.0" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs Gaussian blur on input image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int kernelRadius;
	double sigma;

private:
	enum EPropertyID
	{
		ID_KernelSize,
		ID_Sigma
	};
};

class SobelFilterNodeType : public NodeType
{
public:
	SobelFilterNodeType()
	{
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Sobel filter' node";

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

		nodeConfig.description = "Calculates the first image derivatives using  Sobel operator";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	CannyEdgeDetectorNodeType()
		: threshold(10)
		, ratio(3)
	{
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Canny edge detector' node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Canny(input, output, threshold, threshold*ratio, 3);
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			threshold = newValue.toDouble();
			return true;
		case ID_Ratio:
			ratio = newValue.toDouble();
			return true;
		}

		return false;
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
			{ EPropertyType::Double, "Threshold", QVariant(threshold), "min=0.0;max=100.0;decimals=3" },
			{ EPropertyType::Double, "Ratio", QVariant(ratio), "min=0.0;decimals=3" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Detects edges in input image using Canny detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	double threshold;
	double ratio;

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Ratio,
	};
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
		case Alpha:
			_alpha = newValue.toDouble();
			return true;
		case Beta:
			_beta = newValue.toDouble();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Add' node";
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
			{ EPropertyType::Double, "Alpha", QVariant(_alpha), "min=0.0;max=1.0;decimals=3;step=0.1" },
			{ EPropertyType::Double, "Beta", QVariant(_beta), "min=0.0;max=1.0;decimals=3;step=0.1" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Adds one image to another with specified weights: alpha * x + beta * y";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		Alpha,
		Beta
	};

	double _alpha;
	double _beta;
};

class AbsoluteNodeType : public NodeType
{
public:
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Absolute' node";
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
		qDebug() << "Executing 'Subtract' node";
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
		qDebug() << "Executing 'Absolute difference' node";
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
		qDebug() << "Executing 'Negate' node";
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
		: threshold(128)
		, inv(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			threshold = newValue.toInt();
			return true;
		case ID_Invert:
			inv = newValue.toBool();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Binarization' node";

		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(src.rows == 0 || src.cols == 0 || threshold < 0 || threshold > 255)
		{
			dst = cv::Mat();
			return;
		}

		int type = inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
		cv::threshold(src, dst, (double) threshold, 255, type);
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
			{ EPropertyType::Integer, "Threshold", QVariant(threshold), "min=0;max=255" },
			{ EPropertyType::Boolean, "Inverted", QVariant(inv), "" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 depending on threshold value";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int threshold;
	bool inv;

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Invert
	};
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
		case Coefficients:
			_coeffs = newValue.value<Matrix3x3>();
			return true;
		case ScaleAbs:
			_scaleAbs = newValue.toBool();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Custom Convolution' node";

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
			{ EPropertyType::Matrix, "Coefficients", QVariant::fromValue<Matrix3x3>(_coeffs), "" },
			{ EPropertyType::Boolean, "Scale absolute", QVariant(_scaleAbs), "" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs image convolution";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		Coefficients,
		ScaleAbs
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

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Predefined Convolution' node";

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
			QVariant(QStringList() << "No operation" << "Average" << "Gaussian" << "Mean removal" << 
				"Robert's cross 45" << "Robert's cross 135" << "Laplacian" << "Prewitt horizontal" <<
				"Prewitt vertical" << "Sobel horizontal" << "Sobel vertical"), "index=0" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
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
		: se(cvu::EStructuringElementType::Ellipse)
		, xradius(1)
		, yradius(1)
		, rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_StructuringElementType:
			se = cvu::EStructuringElementType(newValue.toInt());
			return true;
		case ID_XRadius:
			xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Structuring element' node";

		cv::Mat& kernel = writer->lockSocket(0);

		if(xradius == 0 || yradius == 0)
		{
			kernel = cv::Mat();
			return;
		}

		kernel = cvu::standardStructuringElement(xradius, yradius, se, rotation);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ "structuringElement", "Structuring element", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "SE shape", 
				QVariant(QStringList() << "Rectangle" << "Ellipse" << "Cross"), "index=1" },
			{ EPropertyType::Integer, "Horizontal radius", QVariant(xradius), "min=1;max=50" },
			{ EPropertyType::Integer, "Vertical radius", QVariant(yradius), "min=1;max=50" },
			{ EPropertyType::Integer, "Rotation", QVariant(rotation), "min=0;max=359;wrap=true" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
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

private:
	cvu::EStructuringElementType se;
	int xradius;
	int yradius;
	int rotation;
};

class MorphologyNodeType : public NodeType
{
public:
	MorphologyNodeType()
		: op(Erode)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			op = EMorphologyOperation(newValue.toInt());
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Morphology' node";

		const cv::Mat& src = reader->readSocket(0);
		const cv::Mat& se = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if(se.cols == 0 || se.rows == 0 || src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}
		
		cv::morphologyEx(src, dst, op, se);
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
			QVariant(QStringList() << "Erode" << "Dilate" << "Open" << 
			"Close" << "Gradient" << "Top Hat" << "Black Hat"),
			"" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
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

	EMorphologyOperation op;
};

/*class MorphologyNodeType : public NodeType
{
public:
	MorphologyNodeType()
		: op(Erode)
		, se(cvu::EStructuringElementType::Rectangle)
		, xradius(1)
		, yradius(1)
		, rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			op = EMorphologyOperation(newValue.toInt());
			return true;
		case ID_StructuringElementType:
			se = cvu::EStructuringElementType(newValue.toInt());
			return true;
		case ID_XRadius:
			xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Morphology' node";

		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);
		cv::Mat& kernel = writer->lockSocket(1);

		if(xradius == 0 || yradius == 0)
		{
			dst = cv::Mat();
			kernel = cv::Mat();
			return;
		}

		kernel = cvu::standardStructuringElement(xradius, yradius, se, rotation);

		if(src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}
		
		cv::morphologyEx(src, dst, op, kernel);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			// still need to think how to preview binary 0-1 image such as structuring element
			{ "structuringElement", "Structuring element", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
			QVariant(QStringList() << "Erode" << "Dilate" << "Open" << 
				"Close" << "Gradient" << "Top Hat" << "Black Hat"),
			"" },
			{ EPropertyType::Enum, "SE shape", 
			QVariant(QStringList() << "Rectangle" << "Ellipse" << "Cross"), "" },
			{ EPropertyType::Integer, "Horizontal radius", QVariant(xradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Vertical radius", QVariant(yradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Rotation", QVariant(rotation), "min=0;max=359" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
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
		ID_Operation,
		ID_StructuringElementType,
		ID_XRadius,
		ID_YRadius,
		ID_Rotation
	};

private:
	EMorphologyOperation op;
	cvu::EStructuringElementType se;
	int xradius;
	int yradius;
	int rotation;
};
*/

REGISTER_NODE("Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Predefined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Absolute diff.", AbsoluteDifferenceNodeType)
REGISTER_NODE("Subtract", SubtractNodeType)
REGISTER_NODE("Absolute", AbsoluteNodeType)
REGISTER_NODE("Add", AddNodeType)
REGISTER_NODE("Binarization", BinarizationNodeType)
REGISTER_NODE("Structuring element", StructuringElementNodeType)
REGISTER_NODE("Morphology op.", MorphologyNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)
REGISTER_NODE("Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Video from file", VideoFromFileNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)
