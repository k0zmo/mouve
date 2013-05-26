#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "CV.h"

class MixtureOfGaussiansNodeType : public NodeType
{
public:
	MixtureOfGaussiansNodeType()
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

	bool restart() override
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


class GaussianBlurNodeType : public NodeType
{
public:
	GaussianBlurNodeType()
		: _sigma(1.2)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
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

		//int ksize = cvRound(_sigma * 3 * 2 + 1) | 1;
		cv::GaussianBlur(input, output, cv::Size(0,0), _sigma, 0);

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
			{ EPropertyType::Double, "Sigma", "min:0.1, step:0.1" },
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
		ID_Sigma
	};

	double _sigma;
};

/// TODO: aperture size
/// TODO: xorder, yorder
/// Rozbic na Sobel i Sobel edge
class SobelNodeType : public NodeType
{
public:
	SobelNodeType()
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

class LaplacianFilterNodeType : public NodeType
{
public:
	LaplacianFilterNodeType()
		: _apertureSize(1)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_ApertureSize:
			{
				int aperture = newValue.toInt();
				if(aperture == 1 || aperture == 3 || aperture == 5 || aperture == 7)
				{
					_apertureSize = aperture;
					return true;
				}
			}
			break;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_ApertureSize: return _apertureSize;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::Laplacian(input, output, CV_8UC1, _apertureSize);
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
			{ EPropertyType::Integer, "Aperture size", "min:1, max:7, step:2" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Calculates the Laplacian of an image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_ApertureSize
	};

	int _apertureSize;
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

class OtsuThresholdingNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0 || src.type() != CV_8U)
			return ExecutionStatus(EStatus::Ok);

		cv::threshold(src, dst, 0, 255, cv::THRESH_OTSU);

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

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 using histogram";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
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


class ClaheNodeType : public NodeType
{
public:
	ClaheNodeType()
		: _clipLimit(2.0)
		, _tilesGridSize(8)
	{
	}


	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_ClipLimit:
			_clipLimit = newValue.toDouble();
			return true;
		case ID_TilesGridSize:
			_tilesGridSize = newValue.toUInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_ClipLimit: return _clipLimit;
		case ID_TilesGridSize: return _tilesGridSize;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
		clahe->setClipLimit(_clipLimit);
		clahe->setTilesGridSize(cv::Size(_tilesGridSize, _tilesGridSize));

		clahe->apply(src, dst);

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
			{ EPropertyType::Double, "Clip limit", "min:0.0" },
			{ EPropertyType::Integer, "Tiles size", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs CLAHE (Contrast Limited Adaptive Histogram Equalization)";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_ClipLimit,
		ID_TilesGridSize
	};

	double _clipLimit;
	int _tilesGridSize;
};

class DrawHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		cv::Mat& canvas = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Compute histogram
		const int bins = 256;
		const float range[] = { 0, 256 } ;
		const float* histRange = { range };
		const bool uniform = true; 
		const bool accumulate = false;
		cv::Mat hist;
		cv::calcHist(&src, 1, 0, cv::noArray(), hist, 1, &bins, &histRange, uniform, accumulate);

		// Draw histogram
		const int hist_h = 125;
		canvas = cv::Mat::zeros(hist_h, bins, CV_8UC3);
		cv::normalize(hist, hist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());

		for(int j = 0; j < bins-1; ++j)
		{
			cv::line(canvas, 
				cv::Point(j, hist_h),
				cv::Point(j, hist_h - (hist.at<float>(j))),
				cv::Scalar(200,200,200),
				1, 8, 0);
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
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

class BackgroundSubtractorNodeType : public NodeType
{
public:
	BackgroundSubtractorNodeType()
		: _alpha(0.92f)
		, _threshCoeff(3)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Alpha:
			_alpha = newValue.toDouble();
			return true;
		case ID_ThresholdCoeff:
			_threshCoeff = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Alpha: return _alpha;
		case ID_ThresholdCoeff: return _threshCoeff;
		}
		return QVariant();
	}

	bool restart() override
	{
		_frameN1 = cv::Mat();
		_frameN2 = cv::Mat();
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& frame = reader.readSocket(0).getImage();

		cv::Mat& background = writer.acquireSocket(0).getImage();
		cv::Mat& movingPixels = writer.acquireSocket(1).getImage();
		cv::Mat& threshold = writer.acquireSocket(2).getImage();

		if(frame.empty())
			return ExecutionStatus(EStatus::Ok);

		if(!_frameN2.empty()
			&& _frameN2.size() == frame.size())
		{
			if(background.empty()
				|| background.size() != frame.size())
			{
				background.create(frame.size(), CV_8UC1);
				background = cv::Scalar(0);
				movingPixels.create(frame.size(), CV_8UC1);
				movingPixels = cv::Scalar(0);
				threshold.create(frame.size(), CV_8UC1);
				threshold = cv::Scalar(127);
			}

			cvu::parallel_for(cv::Range(0, frame.rows), [&](const cv::Range& range)
			{
				for(int y = range.start; y < range.end; ++y)
				{
					for(int x = 0; x < frame.cols; ++x)
					{
						uchar thresh = threshold.at<uchar>(y, x);
						uchar pix = frame.at<uchar>(y, x);

						// Find moving pixels
						bool moving = std::abs(pix - _frameN1.at<uchar>(y, x)) > thresh
							&& std::abs(pix - _frameN2.at<uchar>(y, x)) > thresh;
						movingPixels.at<uchar>(y, x) = moving ? 255 : 0;

						const int minThreshold = 20;

						if(!moving)
						{
							// Update background image
							uchar newBackground = _alpha*float(background.at<uchar>(y, x)) + (1-_alpha)*float(pix);
							background.at<uchar>(y, x) = newBackground;

							// Update threshold image
							float thresh = _alpha*float(threshold.at<uchar>(y, x)) + 
								(1-_alpha)*(_threshCoeff * std::abs(pix - newBackground));
							if(thresh > float(minThreshold))
								threshold.at<uchar>(y, x) = thresh;
						}
						else
						{
							// Update threshold image
							threshold.at<uchar>(y, x) = minThreshold;
						}
					}
				}
			});
		}

		_frameN2 = _frameN1;
		_frameN1 = frame.clone();

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "background", "Background", "" },
			{ ENodeFlowDataType::Image, "movingPixels", "Moving pixels", "" },
			{ ENodeFlowDataType::Image, "threshold", "Threshold image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Alpha", "min:0.0, max:1.0, decimals:3" },
			{ EPropertyType::Double, "Threshold coeff.", "min:0.0, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID
	{
		ID_Alpha,
		ID_ThresholdCoeff
	};

	cv::Mat _frameN1; // I_{n-1}
	cv::Mat _frameN2; // I_{n-2}
	float _alpha;
	float _threshCoeff;
};

REGISTER_NODE("Video/Background subtraction", BackgroundSubtractorNodeType)
REGISTER_NODE("Video/Mixture of Gaussians", MixtureOfGaussiansNodeType)

REGISTER_NODE("Edge/Canny edge detector", CannyEdgeDetectorNodeType)
REGISTER_NODE("Edge/Sobel", SobelNodeType)
REGISTER_NODE("Edge/Laplacian", LaplacianFilterNodeType)

REGISTER_NODE("Image/Upsample", UpsampleNodeType)
REGISTER_NODE("Image/Downsample", DownsampleNodeType)
REGISTER_NODE("Image/CLAHE", ClaheNodeType)
REGISTER_NODE("Image/Equalize histogram", EqualizeHistogramNodeType)
REGISTER_NODE("Image/Draw histogram", DrawHistogramNodeType)
REGISTER_NODE("Image/Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Image/Predefined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Image/Morphology op.", MorphologyNodeType)
REGISTER_NODE("Image/Gaussian blur", GaussianBlurNodeType)

REGISTER_NODE("Segmentation/Otsu's thresholding", OtsuThresholdingNodeType)
REGISTER_NODE("Segmentation/Binarization", BinarizationNodeType)

REGISTER_NODE("Arithmetic/Negate", NegateNodeType)
REGISTER_NODE("Arithmetic/Absolute diff.", AbsoluteDifferenceNodeType)
//REGISTER_NODE("Subtract", SubtractNodeType)
//REGISTER_NODE("Absolute", AbsoluteNodeType)
REGISTER_NODE("Arithmetic/Add", AddNodeType)

REGISTER_NODE("Source/Structuring element", StructuringElementNodeType)
