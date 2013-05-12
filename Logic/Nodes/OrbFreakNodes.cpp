#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

class OrbFeatureDetectorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::OrbFeatureDetector orb;
		orb.detect(src, kp.kpoints);
		kp.image = src;

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

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class OrbFeatureExtractorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();

		// validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		// outputs
		KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
		outKp = kp;

		cv::OrbDescriptorExtractor orb;
		orb.compute(kp.image, outKp.kpoints, outDescriptors);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class FreakFeatureExtractorNodeType : public NodeType
{
public:
	FreakFeatureExtractorNodeType()
		: _orientationNormalized(true)
		, _scaleNormalized(true)
		, _patternScale(22.0f)
		, _nOctaves(4)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_OrientationNormalized:
			_orientationNormalized = newValue.toBool();
			return true;
		case ID_ScaleNormalized:
			_scaleNormalized = newValue.toBool();
			return true;
		case ID_PatternScale:
			_patternScale = newValue.toFloat();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_OrientationNormalized: return _orientationNormalized;
		case ID_ScaleNormalized: return _scaleNormalized;
		case ID_PatternScale: return _patternScale;
		case ID_NumOctaves: return _nOctaves;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();

		// validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		// outputs
		KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
		outKp = kp;

		cv::FREAK freak(_orientationNormalized, _scaleNormalized, _patternScale, _nOctaves);
		freak.compute(kp.image, outKp.kpoints, outDescriptors);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Boolean, "Enable orientation normalization", "" },
			{ EPropertyType::Boolean, "Enable scale normalization", "" },
			{ EPropertyType::Double, "Scaling of the description pattern", "min:1.0" },
			{ EPropertyType::Integer, "Number of octaves covered by detected keypoints", "min:1" },
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
		ID_OrientationNormalized,
		ID_ScaleNormalized,
		ID_PatternScale,
		ID_NumOctaves,
	};

	bool _orientationNormalized;
	bool _scaleNormalized;
	float _patternScale;
	int _nOctaves;
};

class OrbNodeType : public OrbFeatureDetectorNodeType
{
public:

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// ouputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::ORB orb;
		orb(src, cv::noArray(), kp.kpoints, descriptors);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		OrbFeatureDetectorNodeType::configuration(nodeConfig);

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/Binary/FREAK Extractor", FreakFeatureExtractorNodeType)
REGISTER_NODE("Features/Binary/ORB Extractor", OrbFeatureExtractorNodeType)
REGISTER_NODE("Features/Binary/ORB Detector", OrbFeatureDetectorNodeType)
REGISTER_NODE("Features/Binary/ORB", OrbNodeType)
