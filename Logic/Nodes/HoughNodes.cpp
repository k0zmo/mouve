#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

class HoughLinesNodeType : public NodeType
{
public:
	HoughLinesNodeType()
		: _threshold(100)
		, _rhoResolution(1.0f)
		, _thetaResolution(1.0f)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toInt();
			return true;
		case ID_RhoResolution:
			if(newValue.toFloat() <= 0)
				return false;
			_rhoResolution = newValue.toFloat();
			return true;
		case ID_ThetaResolution:
			if(newValue.toFloat() <= 0)
				return false;
			_thetaResolution = newValue.toFloat();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_RhoResolution: return _rhoResolution;
		case ID_ThetaResolution: return _thetaResolution;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		cv::Mat& lines = writer.acquireSocket(0).getArray();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Vec2f> linesVector;
		cv::HoughLines(src, linesVector, _rhoResolution, CV_PI/180 * _thetaResolution, _threshold);
		lines.create(linesVector.size(), 2, CV_32F);

		float* linesPtr = lines.ptr<float>();
		for(auto& line : linesVector)
		{
			linesPtr[0] = line[0];
			linesPtr[1] = line[1];
			linesPtr += 2;
		}

		std::ostringstream strm;
		strm << "Lines detected: " << linesVector.size();
		return ExecutionStatus(EStatus::Ok, strm.str());
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
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

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_RhoResolution,
		ID_ThetaResolution,
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

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_AccResolution:
			_dp = newValue.toDouble();
			return true;
		case ID_CannyThreshold:
			if(newValue.toDouble() <= 0)
				return false;
			_cannyThreshold = newValue.toDouble();
			return true;
		case ID_CircleCenterThreshold:
			if(newValue.toDouble() <= 0)
				return false;
			_accThreshold = newValue.toDouble();
			return true;
		case ID_MinRadius:
			_minRadius = newValue.toInt();
			return true;
		case ID_MaxRadius:
			_maxRadius = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_AccResolution: return _dp;
		case ID_CannyThreshold: return _cannyThreshold;
		case ID_CircleCenterThreshold: return _accThreshold;
		case ID_MinRadius: return _minRadius;
		case ID_MaxRadius: return _maxRadius;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		cv::Mat& circles = writer.acquireSocket(0).getArray();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::HoughCircles(src, circles, CV_HOUGH_GRADIENT, _dp, 
			src.rows/8, _cannyThreshold, _accThreshold, _minRadius, _maxRadius);

		std::ostringstream strm;
		strm << "Circles detected: " << circles.cols;
		return ExecutionStatus(EStatus::Ok, strm.str());
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "lines", "Lines", "" },
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

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_AccResolution,
		ID_CannyThreshold,
		ID_CircleCenterThreshold,
		ID_MinRadius,
		ID_MaxRadius
	};

	double _dp;
	double _cannyThreshold;
	double _accThreshold;
	int _minRadius;
	int _maxRadius;
};

REGISTER_NODE("Features/Hough Circles", HoughCirclesNodeType)
REGISTER_NODE("Features/Hough Lines", HoughLinesNodeType)