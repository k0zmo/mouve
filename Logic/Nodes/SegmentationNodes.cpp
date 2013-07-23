#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

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
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);
		if(_threshold < 0 || _threshold > 255)
			return ExecutionStatus(EStatus::Error, "Bad threshold value");

		// Do stuff
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

		nodeConfig.description = "Applies a fixed-level threshold to each pixel element.";
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
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(src.empty() || src.type() != CV_8U)
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
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

		nodeConfig.description = "Applies optimal threshold value using Otsu's algorithm to each pixel element.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Segmentation/Otsu's thresholding", OtsuThresholdingNodeType)
REGISTER_NODE("Segmentation/Binarization", BinarizationNodeType)