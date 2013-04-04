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
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& lines = writer.acquireSocket(0).getArray();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Vec2f> linesVector;
		cv::HoughLines(src, linesVector, _rhoResolution, CV_PI/180 / _thetaResolution, _threshold);
		lines.create(linesVector.size(), 2, CV_32F);

		float* linesPtr = lines.ptr<float>();
		for(auto& line : linesVector)
		{
			linesPtr[0] = line[0];
			linesPtr[1] = line[1];
			linesPtr += 2;
		}

		return ExecutionStatus(EStatus::Ok);
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

REGISTER_NODE("Features/Hough Lines", HoughLinesNodeType)