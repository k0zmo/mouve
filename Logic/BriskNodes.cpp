#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

class BriskFeatureDetectorNodeType : public NodeType
{
public:
	BriskFeatureDetectorNodeType()
		: _thresh(30)
		, _nOctaves(3)
		, _patternScale(1.0f)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_thresh = newValue.toInt();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case ID_PatternScale:
			_patternScale  = newValue.toFloat();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _thresh;
		case ID_NumOctaves: return _nOctaves;
		case ID_PatternScale: return _patternScale;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::KeyPoints& keypoints = writer.acquireSocket(0).getKeypoints();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::BRISK detector(_thresh, _nOctaves, _patternScale);
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

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_NumOctaves,
		ID_PatternScale
	};

	int _thresh;
	int _nOctaves;
	float _patternScale;
};

class BriskFeatureExtractorNodeType : public NodeType
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

		cv::BRISK extractor;
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

class BruteForceMatcherNodeType : public NodeType
{
public:
	BruteForceMatcherNodeType()
		: _crossCheck(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_CrossCheck:
			_crossCheck = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_CrossCheck: return _crossCheck;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& queryDescriptors = reader.readSocket(0).getArray();
		const cv::Mat& trainDescriptors = reader.readSocket(1).getArray();

		if(!queryDescriptors.data || !trainDescriptors.data)
			return ExecutionStatus(EStatus::Ok);

		cv::DMatches& matches = writer.acquireSocket(0).getMatches();

		cv::BFMatcher matcher(cv::NORM_HAMMING, _crossCheck);
		matcher.match(queryDescriptors, trainDescriptors, matches);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Array, "descriptors1", "1st descriptors", "" },
			{ ENodeFlowDataType::Array, "descriptors2", "2nd descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Boolean, "Cross check", "" },
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
		ID_CrossCheck
	};

	bool _crossCheck;
};

REGISTER_NODE("Features/BF Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/BRISK Extractor", BriskFeatureExtractorNodeType)
REGISTER_NODE("Features/BRISK Detector", BriskFeatureDetectorNodeType)