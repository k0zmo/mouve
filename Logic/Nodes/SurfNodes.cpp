#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/nonfree/features2d.hpp>

class SurfFeatureDetectorNodeType : public NodeType
{
public:
	SurfFeatureDetectorNodeType()
		: _hessianThreshold(400.0)
		, _nOctaves(4)
		, _nOctaveLayers(2)
		, _extended(true)
		, _upright(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_HessianThreshold:
			_hessianThreshold = newValue.toDouble();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case ID_NumOctaveLayers:
			_nOctaveLayers = newValue.toInt();
			return true;
		case ID_Extended:
			_extended = newValue.toBool();
			return true;
		case ID_Upright:
			_upright = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_HessianThreshold: return _hessianThreshold;
		case ID_NumOctaves: return _nOctaves;
		case ID_NumOctaveLayers: return _nOctaveLayers;
		case ID_Extended: return _extended;
		case ID_Upright: return _upright;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::KeyPoints& keypoints = writer.acquireSocket(0).getKeypoints();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves,
			_nOctaveLayers, _extended, _upright);
		detector.detect(src, keypoints);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "min:1" },
			{ EPropertyType::Integer, "Number of octave layers", "min:1" },
			{ EPropertyType::Boolean, "Extended", "" },
			{ EPropertyType::Boolean, "Upright", "" },
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
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumOctaveLayers,
		ID_Extended,
		ID_Upright
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nOctaveLayers;
	bool _extended;
	bool _upright;
};

class SurfFeatureExtractorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints = reader.readSocket(1).getKeypoints();

		if(keypoints.empty() || imageSrc.cols == 0 || imageSrc.rows == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::Mat& outDescriptors = writer.acquireSocket(0).getArray();
		cv::KeyPoints& outKeypoints = writer.acquireSocket(1).getKeypoints();

		outKeypoints = keypoints;

		cv::SurfDescriptorExtractor extractor;
		extractor.compute(imageSrc, outKeypoints, outDescriptors);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/SURF Extractor", SurfFeatureExtractorNodeType)
REGISTER_NODE("Features/SURF Detector", SurfFeatureDetectorNodeType)
