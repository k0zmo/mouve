#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

#include "CV.h"

class BoxFilterNodeType : public NodeType
{
public:
	BoxFilterNodeType()
		: _kernelSize(3)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::KernelSize:
			_kernelSize = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::KernelSize: return _kernelSize;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::boxFilter(input, output, CV_8UC1, 
			cv::Size(_kernelSize, _kernelSize),
			cv::Point(-1,1), true, cv::BORDER_REFLECT_101);
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
			{ EPropertyType::Integer, "Blurring kernel size", "min:2, step:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Blurs an image using box filter.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid { KernelSize };
	int _kernelSize;
};

class BilateralFilterNodeType : public NodeType
{
public:
	BilateralFilterNodeType()
		: _diameter(9)
		, _sigmaColor(50)
		, _sigmaSpace(50)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Diameter:
			_diameter = newValue.toInt();
			return true;
		case pid::SigmaColor:
			_sigmaColor = newValue.toDouble();
			return true;
		case pid::SigmaSpace:
			_sigmaSpace = newValue.toDouble();
			return true;
		default: return false;
		}
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Diameter: return _diameter;
		case pid::SigmaColor: return _sigmaColor;
		case pid::SigmaSpace: return _sigmaSpace;
		}
		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::bilateralFilter(input, output, _diameter,
			_sigmaColor, _sigmaSpace, cv::BORDER_REFLECT_101);
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
			{ EPropertyType::Integer, "Diameter of pixel neighborhood", "" },
			{ EPropertyType::Double, "Filter sigma in color space", "" },
			{ EPropertyType::Double, "Filter sigma in the coordinate space", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Applies the bilateral filter to an image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid { Diameter, SigmaColor, SigmaSpace };
	int _diameter;
	double _sigmaColor;
	double _sigmaSpace;
};

class GaussianFilterNodeType : public NodeType
{
public:
	GaussianFilterNodeType()
		: _sigma(1.2)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Sigma:
			_sigma = newValue.toDouble();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Sigma: return _sigma;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(!input.data)
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		// sigma = 0.3 * ((ksize-1)*0.5 - 1) + 0.8
		// int ksize = cvRound((20*_sigma - 7)/3);
		//cv::Mat kernel = cv::getGaussianKernel(ksize, _sigma, CV_64F);
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

		nodeConfig.description = "Blurs an image using a Gaussian filter.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Sigma
	};

	double _sigma;
};

class MedianFilterNodeType : public NodeType
{
public:
	MedianFilterNodeType()
		: _apertureSize(3)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize:
			if((newValue.toInt() % 2) != 0)
			{
				_apertureSize = newValue.toInt();
				return true;
			}
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize: return _apertureSize;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::medianBlur(input, output, _apertureSize);
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
			{ EPropertyType::Integer, "Aperture linear size", "min:3, step:2" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Blurs an image using the median filter.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid { ApertureSize };
	int _apertureSize;
};

class LaplacianFilterNodeType : public NodeType
{
public:
	LaplacianFilterNodeType()
		: _apertureSize(1)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize:
		{
			int v = newValue.toInt();
			if((v % 2) != 0 && v <= 31 && v >= 1)
			{
				_apertureSize = v;
				return true;
			}
		}
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize: return _apertureSize;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
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
			{ EPropertyType::Integer, "Aperture size", "min:1, max: 31, step:2" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Calculates the Laplacian of an image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid { ApertureSize };
	int _apertureSize;
};

class SobelFilterNodeType : public NodeType
{
public:
	SobelFilterNodeType()
		: _apertureSize(3)
		, _order(1)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize:
			{
				int v = newValue.toInt();
				if((v % 2) != 0 && v <= 7 && v >= 1)
				{
					_apertureSize = v;
					return true;
				}
			}
			break;
		case pid::Order:
			_order = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ApertureSize: return _apertureSize;
		case pid::Order: return _order;
		}
		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::Mat xgrad;
		cv::Sobel(input, xgrad, CV_16S, _order, 0, _apertureSize);
		cv::convertScaleAbs(xgrad, xgrad);

		cv::Mat ygrad;
		cv::Sobel(input, ygrad, CV_16S, 0, _order, _apertureSize);
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
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Aperture size", "min:1, max: 7, step:2" },
			{ EPropertyType::Integer, "Derivatives order", "min:1, max: 3, step:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Calculates the first image derivatives using Sobel operator.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid { ApertureSize, Order };
	int _apertureSize;
	int _order;
};

class ScharrFilterNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::Mat xgrad;
		cv::Scharr(input, xgrad, CV_16S, 1, 0);
		cv::convertScaleAbs(xgrad, xgrad);

		cv::Mat ygrad;
		cv::Scharr(input, ygrad, CV_16S, 0, 1);
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

		nodeConfig.description = "Calculates the first image derivatives using Scharr operator.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class PredefinedConvolutionNodeType : public NodeType
{
public:
	PredefinedConvolutionNodeType()
		: _filterType(cvu::EPredefinedConvolutionType::NoOperation)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case 0:
			_filterType = newValue.toEnum().cast<cvu::EPredefinedConvolutionType>();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case 0: return _filterType;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::Mat kernel = cvu::predefinedConvolutionKernel(_filterType);
		bool weight0 = cv::sum(kernel) == cv::Scalar(0);
		int ddepth = weight0 ? CV_16S : -1;

		cv::filter2D(input, output, ddepth, kernel);

		if(weight0)
			cv::convertScaleAbs(output, output);

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
			"item: Sobel horizontal, item: Sobel vertical, " 
			"item: Scharr horizontal, item: Scharr vertical"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs image convolution using one of predefined convolution kernel.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	cvu::EPredefinedConvolutionType _filterType;
};

class CustomConvolutionNodeType : public NodeType
{
public:
	CustomConvolutionNodeType()
		: _coeffs(1)
		, _scaleAbs(true)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Coefficients:
			_coeffs = newValue.toMatrix3x3();
			return true;
		case pid::ScaleAbs:
			_scaleAbs = newValue.toBool();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Coefficients: return _coeffs;
		case pid::ScaleAbs: return _scaleAbs;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& input = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		int ddepth = _scaleAbs ? CV_16S : -1;

		cv::Mat kernel(3, 3, CV_64FC1, _coeffs.v);
		cv::filter2D(input, output, ddepth, kernel);

		if(_scaleAbs)
			cv::convertScaleAbs(output, output);

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

		nodeConfig.description = "Performs image convolution using user-given convolution kernel.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Coefficients,
		ScaleAbs
	};

	Matrix3x3 _coeffs;
	bool _scaleAbs;
};

REGISTER_NODE("Filters/Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Filters/Pre-defined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Filters/Scharr filter", ScharrFilterNodeType)
REGISTER_NODE("Filters/Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Filters/Laplacian filter", LaplacianFilterNodeType)
REGISTER_NODE("Filters/Median filter", MedianFilterNodeType)
REGISTER_NODE("Filters/Gaussian filter", GaussianFilterNodeType)
REGISTER_NODE("Filters/Bilateral filter", BilateralFilterNodeType)
REGISTER_NODE("Filters/Box filter", BoxFilterNodeType)
