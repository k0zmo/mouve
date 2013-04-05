#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include "CV.h"

class BayerToGrayNodeType : public NodeType
{
public:
	BayerToGrayNodeType()
		: _BayerCode(cvu::EBayerCode::RG)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case 0:
			_BayerCode = cvu::EBayerCode(newValue.toUInt());
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case 0: return int(_BayerCode);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);
		
		cv::cvtColor(input, output, cvu::bayerCodeGray(_BayerCode));

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
			{ EPropertyType::Enum, "Bayer format", "item: BG, item: GB, item: RG, item: GR" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs demosaicing from Bayer pattern image to gray-scale image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	cvu::EBayerCode _BayerCode;
};

class BayerToRgbNodeType : public BayerToGrayNodeType
{
public:
	BayerToRgbNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImageRgb();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);
		
		cv::cvtColor(input, output, cvu::bayerCodeRgb(_BayerCode));

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		BayerToGrayNodeType::configuration(nodeConfig);

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.description = "Performs demosaicing from Bayer pattern image to RGB image";
	}
};

REGISTER_NODE("Conversion/Gray de-bayer", BayerToGrayNodeType)
REGISTER_NODE("Conversion/RGB de-bayer", BayerToRgbNodeType)