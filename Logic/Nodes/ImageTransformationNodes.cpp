#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

class RotateImageNodeType : public NodeType
{
public:
	RotateImageNodeType()
		: _angle(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Angle: 
			_angle = newValue.toDouble();
			return true;
		}
		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Angle: return _angle;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

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

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Angle
	};
	double _angle;
};

class ScaleImageNodeType : public NodeType
{
public:
	ScaleImageNodeType()
		: _scale(1.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Scale: 
			_scale = newValue.toDouble();
			return true;
		}
		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Scale: return _scale;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::Size dstSize(int(input.cols * _scale), int(input.rows * _scale));
		cv::resize(input, output, dstSize, CV_INTER_AREA);

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Output image width: %d\nOutput image height: %d",
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
		ID_Scale
	};
	double _scale;
};

REGISTER_NODE("Image/Rotate", RotateImageNodeType)
REGISTER_NODE("Image/Scale", ScaleImageNodeType)
