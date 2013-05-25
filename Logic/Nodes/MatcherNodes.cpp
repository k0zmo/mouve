#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"
#include "Kommon/Utils.h"

#include <opencv2/features2d/features2d.hpp>

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
			{
				double th = newValue.toDouble();
				if(th > 0 &&  th < 1.0)
					return false;
				_threshold = th;
			}			
			
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
		// inputs
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& queryDesc = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& trainDesc = reader.readSocket(3).getArray();
		// outputs
		Matches& mt = writer.acquireSocket(0).getMatches();

		// validate inputs
		if(trainDesc.empty() || queryDesc.empty())
			return ExecutionStatus(EStatus::Ok);

		if((size_t) trainDesc.rows != trainKp.kpoints.size()
		|| (size_t) queryDesc.rows != queryKp.kpoints.size())
		{
			return ExecutionStatus(EStatus::Error, "Keypoints and descriptors mismatched");
		}

		int normType = cv::NORM_L2;
		// If BRISK, ORB or FREAK was used as a descriptor
		if(queryDesc.depth() == CV_8U && trainDesc.depth() == CV_8U)
			normType = cv::NORM_HAMMING;

		mt.queryPoints.clear();
		mt.trainPoints.clear();

		cv::BFMatcher matcher(normType, _crossCheck);
		vector<cv::DMatch> matches;
		
		if(fcmp(0.0, _threshold))
		{
			matcher.match(queryDesc, trainDesc, matches);

			for(auto&& match : matches)
			{
				mt.queryPoints.emplace_back(queryKp.kpoints[match.queryIdx].pt);
				mt.trainPoints.emplace_back(trainKp.kpoints[match.trainIdx].pt);
			}
		}
		else
		{
			matcher.match(queryDesc, trainDesc, matches);

			double max_dist = -std::numeric_limits<double>::max();
			double min_dist = +std::numeric_limits<double>::max();

			for(auto&& match : matches)
			{
				double dist = match.distance;
				min_dist = qMin(min_dist, dist);
				max_dist = qMax(max_dist, dist);
			}

			double th = _threshold * min_dist;

			for(auto&& match : matches)
			{
				if(match.distance < th)
				{
					mt.queryPoints.emplace_back(queryKp.kpoints[match.queryIdx].pt);
					mt.trainPoints.emplace_back(trainKp.kpoints[match.trainIdx].pt);
				}
			}
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		std::ostringstream strm;
		strm << "Initial matches found: " << matches.size() << std::endl;
		if(!fcmp(0.0, _threshold))
			strm << "Matches after thresholding found: " << mt.queryPoints.size() << std::endl;

		return ExecutionStatus(EStatus::Ok, strm.str());
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints1", "Query keypoints", "" },
			{ ENodeFlowDataType::Array, "descriptors1", "Query descriptors", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints2", "Train keypoints", "" },
			{ ENodeFlowDataType::Array, "descriptors2", "Train descriptors", "" },
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

class NearestNeighborDistanceRatioMatcherNodeType : public NodeType
{
public:
	NearestNeighborDistanceRatioMatcherNodeType()
		: _distanceRatio(0.8)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_DistanceRatio:
			_distanceRatio = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_DistanceRatio: return _distanceRatio;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& queryDesc = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& trainDesc = reader.readSocket(3).getArray();
		// outputs
		Matches& mt = writer.acquireSocket(0).getMatches();

		// validate inputs
		if(trainDesc.empty() || queryDesc.empty())
			return ExecutionStatus(EStatus::Ok);

		if((size_t) trainDesc.rows != trainKp.kpoints.size()
		|| (size_t) queryDesc.rows != queryKp.kpoints.size())
		{
			return ExecutionStatus(EStatus::Error, "Keypoints and descriptors mismatched");
		}

		cv::Ptr<cv::flann::IndexParams> indexParams = new cv::flann::KDTreeIndexParams();
		cv::Ptr<cv::flann::SearchParams> searchParams = new cv::flann::SearchParams();

		if(trainDesc.depth() == CV_8U)
		{
			indexParams->setAlgorithm(cvflann::FLANN_INDEX_LSH);
			searchParams->setAlgorithm(cvflann::FLANN_INDEX_LSH);
		}

		cv::FlannBasedMatcher matcher(indexParams, searchParams);
		vector<vector<cv::DMatch>> knMatches;
		matcher.knnMatch(queryDesc, trainDesc, knMatches, 2);
		
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(auto&& knMatch : knMatches)
		{
			if(knMatch.size() != 2)
				continue;

			auto&& best = knMatch[0];
			auto&& good = knMatch[1];

			if(best.distance <= _distanceRatio * good.distance)
			{
				mt.queryPoints.emplace_back(queryKp.kpoints[best.queryIdx].pt);
				mt.trainPoints.emplace_back(trainKp.kpoints[best.trainIdx].pt);
			}
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Initial matches found: %d\nNNDR Matches found: %d",
				(int) knMatches.size(), (int) mt.queryPoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints1", "Query keypoints", "" },
			{ ENodeFlowDataType::Array, "descriptors1", "Query descriptors", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints2", "Train keypoints", "" },
			{ ENodeFlowDataType::Array, "descriptors2", "Train descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "ID_DistanceRatio", "min:0.0, max:1.0, step:0.1" },
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
		ID_DistanceRatio
	};

	double _distanceRatio;
};

REGISTER_NODE("Features/BForce Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/NNDR Matcher", NearestNeighborDistanceRatioMatcherNodeType)
