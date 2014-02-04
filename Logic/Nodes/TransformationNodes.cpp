#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

#include <opencv2/imgproc/imgproc.hpp>

class RotateImageNodeType : public NodeType
{
public:
	RotateImageNodeType()
		: _angle(0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Angle: 
			_angle = newValue.toDouble();
			return true;
		}
		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Angle: return _angle;
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
		cv::Point2f center(input.cols * 0.5f, input.rows * 0.5f);
		cv::Mat rotationMat = cv::getRotationMatrix2D(center, _angle, 1);
		cv::warpAffine(input, output, rotationMat, input.size(), cv::INTER_CUBIC);

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
			{ EPropertyType::Double, "Angle", "min:0, max:360, step:0.1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Applies rotation transformation to a given image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Angle
	};

	double _angle;
};

class ScaleImageNodeType : public NodeType
{
public:
	ScaleImageNodeType()
		: _scale(1.0)
		, _inter(EInterpolationMethod::eimArea)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Scale: 
			_scale = newValue.toDouble();
			return true;
		case pid::IntepolationMethod:
			_inter = newValue.toEnum().cast<EInterpolationMethod>();
			return true;
		}
		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Scale: return _scale;
		case pid::IntepolationMethod: return _inter;
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
		cv::Size dstSize(int(input.cols * _scale), int(input.rows * _scale));
		cv::resize(input, output, dstSize, 0, 0, int(_inter));

		return ExecutionStatus(EStatus::Ok, 
			string_format("Output image width: %d\nOutput image height: %d",
				output.cols, output.rows));
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
			{ EPropertyType::Double, "Scale", "min:0" },
			{ EPropertyType::Enum, "Interpolation method", 
			"item: Nearest neighbour, item: Linear, item: Cubic, item: Area, item: Lanczos" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Resizes given image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Scale,
		IntepolationMethod
	};

	enum class EInterpolationMethod
	{
		eimNearestNeighbour = cv::INTER_NEAREST,
		eimLinear           = cv::INTER_LINEAR,
		eimCubic            = cv::INTER_CUBIC,
		eimArea             = cv::INTER_AREA,
		eimLanczos          = cv::INTER_LANCZOS4
	};

	double _scale;
	EInterpolationMethod _inter;
};

class DownsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Acquire output sockets
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::pyrDown(src, dst, cv::Size(src.cols/2, src.rows/2));

		return ExecutionStatus(EStatus::Ok, 
			string_format("Image size: %dx%d\n", dst.cols, dst.rows));
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

		nodeConfig.description = "Blurs an image and downsamples it.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class UpsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Acquire output sockets
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::pyrUp(src, dst, cv::Size(src.cols*2, src.rows*2));

		return ExecutionStatus(EStatus::Ok, 
			string_format("Image size: %dx%d\n", dst.cols, dst.rows));
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

		nodeConfig.description = "Upsamples an image and then blurs it.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class MaskedImageNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		const cv::Mat& mask = reader.readSocket(1).getImageMono();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(src.empty() || mask.empty())
			return ExecutionStatus(EStatus::Ok);
		if(src.size() != mask.size())
			return ExecutionStatus(EStatus::Error, "Source");

		// Do stuff
		dst = cv::Mat(dst.size(), src.type(), cv::Scalar(0));
		src.copyTo(dst, mask);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::ImageMono, "mask", "Mask", "" },
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

REGISTER_NODE("Transformations/Masked image", MaskedImageNodeType)
REGISTER_NODE("Transformations/Upsample", UpsampleNodeType)
REGISTER_NODE("Transformations/Downsample", DownsampleNodeType)
REGISTER_NODE("Transformations/Rotate", RotateImageNodeType)
REGISTER_NODE("Transformations/Scale", ScaleImageNodeType)
