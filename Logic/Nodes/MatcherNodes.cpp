#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

static void matchImpl(const cv::DescriptorMatcher& matcher, 
					  const cv::Mat& queryDescriptors,
					  const cv::Mat& trainDescriptors,
					  cv::DMatches& matches,
					  double threshold)
{
	if(qFuzzyCompare(0.0, threshold))
	{
		matcher.match(queryDescriptors, trainDescriptors, matches);
	}
	else
	{
		cv::DMatches tmpMatches;
		matcher.match(queryDescriptors, trainDescriptors, tmpMatches);

		double max_dist = -std::numeric_limits<double>::max();
		double min_dist = +std::numeric_limits<double>::max();

		for(int i = 0; i < queryDescriptors.rows; ++i)
		{
			double dist = tmpMatches[i].distance;
			min_dist = qMin(min_dist, dist);
			max_dist = qMax(max_dist, dist);
		}

		matches.clear();

		double th = threshold * min_dist;

		for(int i = 0; i < queryDescriptors.rows; ++i)
		{
			if(tmpMatches[i].distance < th)
				matches.push_back(tmpMatches[i]);
		}
	}
}

class BruteForceMatcherNodeType : public NodeType
{
public:
	BruteForceMatcherNodeType()
		: _crossCheck(false)
		, _threshold(2.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_CrossCheck:
			_crossCheck = newValue.toBool();
			return true;
		case ID_Threshold:
			_threshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_CrossCheck: return _crossCheck;
		case ID_Threshold: return _threshold;
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

		int normType = cv::NORM_L2;
		if(queryDescriptors.depth() == CV_8U && trainDescriptors.depth() == CV_8U)
			normType = cv::NORM_HAMMING;

		cv::BFMatcher matcher(normType, _crossCheck);
		matchImpl(matcher, queryDescriptors, trainDescriptors, matches, _threshold);

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
			{ EPropertyType::Double, "Threshold", "min:0, step:0.1" },
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
		ID_CrossCheck,
		ID_Threshold
	};

	bool _crossCheck;
	double _threshold;
};

class NearestNeighborMatcherNodeType : public NodeType
{
public:
	NearestNeighborMatcherNodeType()
		: _threshold(2.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
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

		cv::FlannBasedMatcher matcher;
		matchImpl(matcher, queryDescriptors, trainDescriptors, matches, _threshold);

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
			{ EPropertyType::Double, "Threshold", "min:0, step:0.1" },
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
		ID_Threshold
	};

	double _threshold;
};

REGISTER_NODE("Features/BForce Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/1NN Matcher", NearestNeighborMatcherNodeType)