#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	CannyEdgeDetectorNodeType()
		: _threshold(10)
		, _ratio(3)
	{
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold: return _threshold;
		case pid::Ratio: return _ratio;
		}

		return NodeProperty();
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold:
			_threshold = newValue.toDouble();
			return true;
		case pid::Ratio:
			_ratio = newValue.toDouble();
			return true;
		}

		return false;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImageMono();

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
			{ ENodeFlowDataType::ImageMono, "output", "Output", "" },
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
	enum class pid
	{
		Threshold,
		Ratio,
	};

	double _threshold;
	double _ratio;
};

class HoughLinesNodeType : public NodeType
{
public:
	HoughLinesNodeType()
		: _threshold(100)
		, _rhoResolution(1.0f)
		, _thetaResolution(1.0f)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold:
			_threshold = newValue.toInt();
			return true;
		case pid::RhoResolution:
			if(newValue.toFloat() <= 0)
				return false;
			_rhoResolution = newValue.toFloat();
			return true;
		case pid::ThetaResolution:
			if(newValue.toFloat() <= 0)
				return false;
			_thetaResolution = newValue.toFloat();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold: return _threshold;
		case pid::RhoResolution: return _rhoResolution;
		case pid::ThetaResolution: return _thetaResolution;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImageMono();
		// Acquire output sockets
		cv::Mat& lines = writer.acquireSocket(0).getArray();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		std::vector<cv::Vec2f> linesVector;
		cv::HoughLines(src, linesVector, _rhoResolution, CV_PI/180 * _thetaResolution, _threshold);
		lines.create(static_cast<int>(linesVector.size()), 2, CV_32F);

		float* linesPtr = lines.ptr<float>();
		for(const auto& line : linesVector)
		{
			linesPtr[0] = line[0];
			linesPtr[1] = line[1];
			linesPtr += 2;
		}

		return ExecutionStatus(EStatus::Ok,
			string_format("Lines detected: %d", (int) linesVector.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageMono, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "lines", "Lines", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Accumulator threshold", "" },
			{ EPropertyType::Double, "Rho resolution", "min:0.01" },
			{ EPropertyType::Double, "Theta resolution", "min:0.01" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Finds lines in a binary image using the standard Hough transform.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Threshold,
		RhoResolution,
		ThetaResolution,
	};

	int _threshold;
	float _rhoResolution;
	float _thetaResolution;
};

class HoughCirclesNodeType : public NodeType
{
public:
	HoughCirclesNodeType()
		: _dp(2.0)
		, _cannyThreshold(200.0)
		, _accThreshold(100.0)
		, _minRadius(0)
		, _maxRadius(0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::AccResolution:
			_dp = newValue.toDouble();
			return true;
		case pid::CannyThreshold:
			if(newValue.toDouble() <= 0)
				return false;
			_cannyThreshold = newValue.toDouble();
			return true;
		case pid::CircleCenterThreshold:
			if(newValue.toDouble() <= 0)
				return false;
			_accThreshold = newValue.toDouble();
			return true;
		case pid::MinRadius:
			_minRadius = newValue.toInt();
			return true;
		case pid::MaxRadius:
			_maxRadius = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::AccResolution: return _dp;
		case pid::CannyThreshold: return _cannyThreshold;
		case pid::CircleCenterThreshold: return _accThreshold;
		case pid::MinRadius: return _minRadius;
		case pid::MaxRadius: return _maxRadius;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImageMono();
		// Acquire output sockets
		cv::Mat& circles = writer.acquireSocket(0).getArray();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::HoughCircles(src, circles, CV_HOUGH_GRADIENT, _dp, 
			src.rows/8, _cannyThreshold, _accThreshold, _minRadius, _maxRadius);

		return ExecutionStatus(EStatus::Ok, 
			string_format("Circles detected: %d", circles.cols));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageMono, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "circles", "Circles", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Accumulator resolution", "min:0.01" },
			{ EPropertyType::Double, "Canny threshold", "min:0.0" },
			{ EPropertyType::Double, "Centers threshold", "min:0.01" },
			{ EPropertyType::Integer, "Min. radius", "min: 0" },
			{ EPropertyType::Integer, "Max. radius", "min: 0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Finds circles in a grayscale image using the Hough transform.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		AccResolution,
		CannyThreshold,
		CircleCenterThreshold,
		MinRadius,
		MaxRadius
	};

	double _dp;
	double _cannyThreshold;
	double _accThreshold;
	int _minRadius;
	int _maxRadius;
};

REGISTER_NODE("Features/Hough Circles", HoughCirclesNodeType)
REGISTER_NODE("Features/Hough Lines", HoughLinesNodeType)
REGISTER_NODE("Features/Canny edge detector", CannyEdgeDetectorNodeType)
