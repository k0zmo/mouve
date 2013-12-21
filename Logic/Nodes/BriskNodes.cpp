#if 0
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>
using std::unique_ptr;

class BriskFeatureDetectorNodeType : public NodeType
{
public:
	BriskFeatureDetectorNodeType()
		: _thresh(30)
		, _nOctaves(3)
		, _patternScale(1.0f)
		, _brisk(new cv::BRISK())
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		if(propId > ID_PatternScale || propId < ID_Threshold)
			return false;

		switch(propId)
		{
		case ID_Threshold:
			_thresh = newValue.toInt();
			break;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			break;
		case ID_PatternScale:
			_patternScale  = newValue.toFloat();
			break;
		}

		// That's a bit cheating here - creating BRISK object takes time (approx. 200ms for PAL)
		// which if done per frame makes it slowish (more than SIFT actually)
		_brisk = unique_ptr<cv::BRISK>(new cv::BRISK(_thresh, _nOctaves, _patternScale));
		return true;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _thresh;
		case ID_NumOctaves: return _nOctaves;
		case ID_PatternScale: return _patternScale;
		}

		return NodeProperty();
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

		_brisk->detect(src, kp.kpoints);
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
			{ EPropertyType::Integer, "FAST/AGAST detection threshold score", "min:1" },
			{ EPropertyType::Integer, "Number of octaves", "min:0" },
			{ EPropertyType::Double, "Pattern scale", "min:0.0" },
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
		ID_Threshold,
		ID_NumOctaves,
		ID_PatternScale
	};

	int _thresh;
	int _nOctaves;
	float _patternScale;
	// Must be pointer since BRISK doesn't implement copy/move operator (they should have)
	unique_ptr<cv::BRISK> _brisk;
};

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
	BriskDescriptorExtractorNodeType()
		: _brisk(new cv::BRISK())
	{
	}

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

		_brisk->compute(kp.image, outKp.kpoints, outDescriptors);

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

private:
	// Must be pointer since BRISK doesn't implement copy/move operator (they should have)
	unique_ptr<cv::BRISK> _brisk;
};

class BriskNodeType : public BriskFeatureDetectorNodeType
{
public:

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// ouputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		(*_brisk)(src, cv::noArray(), kp.kpoints, descriptors);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		BriskFeatureDetectorNodeType::configuration(nodeConfig);

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/Descriptors/BRISK", BriskDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/BRISK", BriskFeatureDetectorNodeType)
REGISTER_NODE("Features/BRISK", BriskNodeType)
#endif