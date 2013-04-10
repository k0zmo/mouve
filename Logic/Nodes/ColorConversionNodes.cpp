#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
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

private:
	cvu::EBayerCode _BayerCode;
};

class BayerToRgbNodeType : public NodeType
{
public:
	BayerToRgbNodeType()
		: _redGain(1.0)
		, _greenGain(1.0)
		, _blueGain(1.0)
		, _BayerCode(cvu::EBayerCode::RG)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_BayerFormat:
			_BayerCode = cvu::EBayerCode(newValue.toUInt());
			return true;
		case ID_RedGain:
			_redGain = newValue.toDouble();
			return true;
		case ID_GreenGain:
			_greenGain = newValue.toDouble();
			return true;
		case ID_BlueGain:
			_blueGain = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_BayerFormat: return int(_BayerCode);
		case ID_RedGain: return _redGain;
		case ID_GreenGain: return _greenGain;
		case ID_BlueGain: return _blueGain;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImageRgb();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);
		
		cv::cvtColor(input, output, cvu::bayerCodeRgb(_BayerCode));

		if(!qFuzzyCompare(_redGain, 1.0)
			|| !qFuzzyCompare(_greenGain, 1.0) 
			|| !qFuzzyCompare(_blueGain, 1.0))
		{
			cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range) 
			{
				for(int y = range.start; y < range.end; ++y)
				{
					for(int x = 0; x < output.cols; ++x)
					{
						cv::Vec3b rgb = output.at<cv::Vec3b>(y, x);
						rgb[0] = cv::saturate_cast<uchar>(rgb[0] * _blueGain);
						rgb[1] = cv::saturate_cast<uchar>(rgb[1] * _greenGain);
						rgb[2] = cv::saturate_cast<uchar>(rgb[2] * _redGain);
						output.at<cv::Vec3b>(y, x) = rgb;
					}
				}
			});
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Bayer format", "item: BG, item: GB, item: RG, item: GR" },
			{ EPropertyType::Double, "Red gain", "min:0.0, max:4.0, step:0.001" },
			{ EPropertyType::Double, "Green gain", "min:0.0, max:4.0, step:0.001" },
			{ EPropertyType::Double, "Blue gain", "min:0.0, max:4.0, step:0.001" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs demosaicing from Bayer pattern image to RGB image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;	
	}

private:
	enum EPropertyID
	{
		ID_BayerFormat,
		ID_RedGain,
		ID_GreenGain,
		ID_BlueGain,
	};

	double _redGain;
	double _greenGain;
	double _blueGain;
	cvu::EBayerCode _BayerCode;
};

class ContrastAndBrightnessNodeType : public NodeType
{
public:
	ContrastAndBrightnessNodeType()
		: _gain(1.0)
		, _bias(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Gain:
			_gain = newValue.toDouble();
			return true;
		case ID_Bias:
			_bias = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Gain: return _gain;
		case ID_Bias: return _bias;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		if(!qFuzzyCompare(_gain, 1.0) || _bias != 0)
		{
			output.create(input.size(), CV_8UC1);

			cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range)
			{
				for(int y = range.start; y < range.end; ++y)
				{
					for(int x = 0; x < output.cols; ++x)
					{
						output.at<uchar>(y, x) = cv::saturate_cast<uchar>(
							_gain * input.at<uchar>(y, x) + _bias);
					}
				}
			});
		}
		else
		{
			output = input.clone();
		}

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
			{ EPropertyType::Double, "Gain", "min:0.0, max:16.0" },
			{ EPropertyType::Integer, "Bias", "min:-255, max:255" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Adjust contrast and brightness of input gray image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;	
	}


protected:
	enum EPropertyID
	{
		ID_Gain,
		ID_Bias
	};

	double _gain;
	int _bias;
};

class ContrastAndBrightnessRgbNodeType : public ContrastAndBrightnessNodeType
{
public:
	ContrastAndBrightnessRgbNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImageRgb();
		cv::Mat& output = writer.acquireSocket(0).getImageRgb();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		if(!qFuzzyCompare(_gain, 1.0) || _bias != 0)
		{
			output.create(input.size(), CV_8UC3);

			cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range)
			{
				for(int y = range.start; y < range.end; ++y)
				{
					for(int x = 0; x < output.cols; ++x)
					{
						cv::Vec3b rgb = input.at<cv::Vec3b>(y, x);
						rgb[0] = cv::saturate_cast<uchar>(_gain * rgb[0] + _bias);
						rgb[1] = cv::saturate_cast<uchar>(_gain * rgb[1] + _bias);
						rgb[2] = cv::saturate_cast<uchar>(_gain * rgb[2] + _bias);
						output.at<cv::Vec3b>(y, x) = rgb;
					}
				}
			});
		}
		else
		{
			output = input.clone();
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		ContrastAndBrightnessNodeType::configuration(nodeConfig);

		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageRgb, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Adjust contrast and brightness of input color image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Conversion/Contrast & brightness RGB", ContrastAndBrightnessRgbNodeType)
REGISTER_NODE("Conversion/Contrast & brightness", ContrastAndBrightnessNodeType)
REGISTER_NODE("Conversion/Gray de-bayer", BayerToGrayNodeType)
REGISTER_NODE("Conversion/RGB de-bayer", BayerToRgbNodeType)
