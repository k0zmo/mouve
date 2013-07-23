#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/nonfree/features2d.hpp>

class SiftFeatureDetectorNodeType : public NodeType
{
public:
	SiftFeatureDetectorNodeType()
		: _nFeatures(0)
		, _nOctaveLayers(3)
		, _contrastThreshold(0.04)
		, _edgeThreshold(10)
		, _sigma(1.6)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_NumFeatures: 
			_nFeatures = newValue.toInt();
			return true;
		case ID_NumOctaveLayers:  
			_nOctaveLayers = newValue.toInt();
			return true;
		case ID_ContrastThreshold:
			_contrastThreshold = newValue.toDouble();
			return true;
		case ID_EdgeThreshold:
			_edgeThreshold = newValue.toDouble();
			return true;
		case ID_Sigma:
			_sigma = newValue.toDouble();
			return true;
		}
		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_NumFeatures: return _nFeatures;
		case ID_NumOctaveLayers: return _nOctaveLayers;
		case ID_ContrastThreshold: return _contrastThreshold;
		case ID_EdgeThreshold: return _edgeThreshold;
		case ID_Sigma: return _sigma;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::SiftFeatureDetector detector(_nFeatures, _nOctaveLayers,
			_contrastThreshold, _edgeThreshold, _sigma);
		detector.detect(src, kp.kpoints);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
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
			{ EPropertyType::Integer, "Number of best features to retain", "min:0" },
			{ EPropertyType::Integer, "Number of layers in each octave", "min:2" },
			{ EPropertyType::Double, "Contrast threshold", "" },
			{ EPropertyType::Double, "Edge threshold", "" },
			{ EPropertyType::Double, "Input image sigma", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Extracts keypoints using difference of gaussians (DoG) algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum EPropertyID
	{
		ID_NumFeatures,
		ID_NumOctaveLayers,
		ID_ContrastThreshold,
		ID_EdgeThreshold,
		ID_Sigma
	};

	int _nFeatures;
	int _nOctaveLayers;
	double _contrastThreshold;
	double _edgeThreshold;
	double _sigma;
};

class SiftDescriptorExtractorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();

		// Validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		// Acquire output sockets
		KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
		outKp = kp;

		// Do stuff
		cv::SiftDescriptorExtractor extractor;
		extractor.compute(kp.image, outKp.kpoints, outDescriptors);

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

		nodeConfig.description = "Computes descriptors using the Scale Invariant Feature Transform (SIFT) algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class SiftNodeType : public SiftFeatureDetectorNodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::SIFT sift(_nFeatures, _nOctaveLayers,
			_contrastThreshold, _edgeThreshold, _sigma);
		sift(src, cv::noArray(), kp.kpoints, descriptors);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		SiftFeatureDetectorNodeType::configuration(nodeConfig);

		// Just add one more output socket
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Extracts keypoints and computes descriptors using the Scale Invariant Feature Transform (SIFT) algorithm.";
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/Descriptors/SIFT", SiftDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/SIFT", SiftFeatureDetectorNodeType)
REGISTER_NODE("Features/SIFT", SiftNodeType)