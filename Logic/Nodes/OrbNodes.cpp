#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

class OrbFeatureDetectorNodeType : public NodeType
{
public:
	OrbFeatureDetectorNodeType()
		: _nfeatures(1000)
		, _scaleFactor(1.2f)
		, _nlevels(8)
		, _edgeThreshold(31)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_NumFeatures:
			_nfeatures = newValue.toInt();
			return true;
		case ID_ScaleFactor:
			_scaleFactor = newValue.toFloat();
			return true;
		case ID_NumLevels:
			_nlevels = newValue.toInt();
			return true;
		case ID_EdgeThreshold:
			_edgeThreshold = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_NumFeatures: return _nfeatures;
		case ID_ScaleFactor: return _scaleFactor;
		case ID_NumLevels: return _nlevels;
		case ID_EdgeThreshold: return _edgeThreshold;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::OrbFeatureDetector orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
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
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Number of features to retain", "min:1" },
			{ EPropertyType::Double, "Pyramid decimation ratio", "min:1.0" },
			{ EPropertyType::Integer, "The number of pyramid levels", "min:1" },
			{ EPropertyType::Integer, "Border margin", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum EPropertyID
	{
		ID_NumFeatures,
		ID_ScaleFactor,
		ID_NumLevels,
		ID_EdgeThreshold
	};

	int _nfeatures;
	float _scaleFactor;
	int _nlevels;
	int _edgeThreshold;
};

class OrbDescriptorExtractorNodeType : public NodeType
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

		cv::ORB orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
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

REGISTER_NODE("Features/Descriptors/ORB", OrbDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/ORB", OrbFeatureDetectorNodeType)
REGISTER_NODE("Features/ORB", OrbNodeType)