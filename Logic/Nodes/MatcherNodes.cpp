#include "NodeType.h"
#include "NodeFactory.h"
#include "Kommon/Utils.h"

#include <opencv2/features2d/features2d.hpp>

using std::vector;

static vector<cv::DMatch> distanceRatioTest(const vector<vector<cv::DMatch>>& knMatches, 
											float distanceRatioThreshold)
{
	vector<cv::DMatch> positiveMatches;
	positiveMatches.reserve(knMatches.size());

	for(const auto& knMatch : knMatches)
	{
		if(knMatch.size() != 2)
			continue;

		const auto& best = knMatch[0];
		const auto& good = knMatch[1];

		if(best.distance <= distanceRatioThreshold * good.distance)
			positiveMatches.push_back(best);
	}

	return positiveMatches;
}

static vector<cv::DMatch> symmetryTest(const vector<cv::DMatch>& matches1to2,
									   const vector<cv::DMatch>& matches2to1)
{
	vector<cv::DMatch> bothMatches;

	for(const auto& match1to2 : matches1to2)
	{
		for(const auto& match2to1 : matches2to1)
		{
			if(match1to2.queryIdx == match2to1.trainIdx
			&& match2to1.queryIdx == match1to2.trainIdx)
			{
				bothMatches.push_back(match1to2);
				break;
			}
		}
	}

	return bothMatches;
}

class MatcherNodeType : public NodeType
{
public:
	MatcherNodeType()
		: _distanceRatio(0.8)
		, _symmetryTest(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_DistanceRatio:
			_distanceRatio = newValue.toDouble();
			return true;
		case ID_SymmetryTest:
			_symmetryTest = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_DistanceRatio: return _distanceRatio;
		case ID_SymmetryTest: return _symmetryTest;
		}

		return QVariant();
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
			{ EPropertyType::Double, "Distance ratio", "min:0.0, max:1.0, step:0.1, decimals:2" },
			{ EPropertyType::Boolean, "Symmetry test", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Finds best matches between query and train descriptors using nearest neighbour distance ratio and/or symmetry test.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum EPropertyID
	{
		ID_DistanceRatio,
		ID_SymmetryTest,
	};

	double _distanceRatio;
	bool _symmetryTest;
};

class BruteForceMatcherNodeType : public MatcherNodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& query = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& train = reader.readSocket(3).getArray();
		// Acquire output sockets
		Matches& mt = writer.acquireSocket(0).getMatches();

		// Validate inputs
		if(train.empty() || query.empty())
			return ExecutionStatus(EStatus::Ok);

		if(query.cols != train.cols
		|| query.type() != train.type())
			return ExecutionStatus(EStatus::Error, "Query and train descriptors are different types");

		if(query.type() != CV_32F && query.type() != CV_8U)
		{
			return ExecutionStatus(EStatus::Error, 
				"Unsupported descriptor data type "
				"(must be float for L2 norm or Uint8 for Hamming norm");
		}

		// Do stuff
		vector<cv::DMatch> matches;

		if(_symmetryTest)
		{
			vector<cv::DMatch> matches1to2 = nndrMatch(query, train);
			vector<cv::DMatch> matches2to1 = nndrMatch(train, query);
			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			matches = nndrMatch(query, train);
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		for(const auto& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Matches found: %d", (int) mt.queryPoints.size()));
	}

private:
	vector<cv::DMatch> nndrMatch(const cv::Mat& query, const cv::Mat& train)
	{
		vector<cv::DMatch> nndrMatches;

		if(query.type() == CV_32F)
		{
			// for each descriptors in query array
			for(int queryIdx = 0; queryIdx < query.rows; ++queryIdx)
			{
				const float* query_row = query.ptr<float>(queryIdx);

				float bestDistance1 = std::numeric_limits<float>::max();
				float bestDistance2 = std::numeric_limits<float>::max();
				int bestTrainIdx1 = -1;

				// for each descriptors in train array
				for(int trainIdx = 0; trainIdx < train.rows; ++trainIdx)
				{
					const float* train_row = train.ptr<float>(trainIdx);

					cv::L2<float> op;
					cv::L2<float>::ResultType dist = op(train_row, query_row, train.cols);

					if(dist < bestDistance1)
					{
						bestDistance2 = bestDistance1;
						bestDistance1 = dist;
						bestTrainIdx1 = trainIdx;
					}
					else if(dist < bestDistance2)
					{
						bestDistance2 = dist;
					}
				}

				if(bestDistance1 <= _distanceRatio * bestDistance2)
				{
					nndrMatches.push_back(cv::DMatch(queryIdx, bestTrainIdx1, bestDistance1));
				}
			}
		}
		else if(query.type() == CV_8U)
		{
			// for each descriptors in query array
			for(int queryIdx = 0; queryIdx < query.rows; ++queryIdx)
			{
				const uchar* query_row = query.ptr<uchar>(queryIdx);

				int bestDistance1 = std::numeric_limits<int>::max();
				int bestDistance2 = std::numeric_limits<int>::max();
				int bestTrainIdx1 = -1;

				// for each descriptors in train array
				for(int trainIdx = 0; trainIdx < train.rows; ++trainIdx)
				{
					const uchar* train_row = train.ptr<uchar>(trainIdx);

					cv::Hamming op;
					cv::Hamming::ResultType dist = op(train_row, query_row, train.cols);

					if(dist < bestDistance1)
					{
						bestDistance2 = bestDistance1;
						bestDistance1 = dist;
						bestTrainIdx1 = trainIdx;
					}
					else if(dist < bestDistance2)
					{
						bestDistance2 = dist;
					}
				}

				if(bestDistance1 <= _distanceRatio * bestDistance2)
				{
					nndrMatches.emplace_back(cv::DMatch(queryIdx, bestTrainIdx1, bestDistance1));
				}
			}
		}

		return nndrMatches;
	}
};

class AproximateNearestNeighborMatcherNodeType : public MatcherNodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& queryDesc = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& trainDesc = reader.readSocket(3).getArray();
		// Acquire output sockets
		Matches& mt = writer.acquireSocket(0).getMatches();

		// Validate inputs
		if(trainDesc.empty() || queryDesc.empty())
			return ExecutionStatus(EStatus::Ok);

		if((size_t) trainDesc.rows != trainKp.kpoints.size()
		|| (size_t) queryDesc.rows != queryKp.kpoints.size())
		{
			return ExecutionStatus(EStatus::Error, "Keypoints and descriptors mismatched");
		}

		// Do stuff
		cv::Ptr<cv::flann::IndexParams> indexParams;
		
		if(trainDesc.depth() == CV_8U)
		{
			indexParams = new cv::flann::LshIndexParams(12, 20, 2);
			// For now doesn't work for binary descriptors :(
			//cv::flann::HierarchicalClusteringIndexParams();
		}
		else if(trainDesc.depth() == CV_32F 
			 || trainDesc.depth() == CV_64F)
		{
			indexParams = new cv::flann::HierarchicalClusteringIndexParams();
		}
		
		cv::FlannBasedMatcher matcher(indexParams);
		vector<cv::DMatch> matches;

		if(_symmetryTest)
		{
			vector<vector<cv::DMatch>> knMatches1to2, knMatches2to1;

			matcher.knnMatch(queryDesc, trainDesc, knMatches1to2, 2);
			matcher.knnMatch(trainDesc, queryDesc, knMatches2to1, 2);

			auto matches1to2 = distanceRatioTest(knMatches1to2, _distanceRatio);
			auto matches2to1 = distanceRatioTest(knMatches2to1, _distanceRatio);
			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			vector<vector<cv::DMatch>> knMatches;
			matcher.knnMatch(queryDesc, trainDesc, knMatches, 2);

			matches = distanceRatioTest(knMatches, _distanceRatio);
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(const auto& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Matches found: %d", (int) mt.queryPoints.size()));
	}
};

/// Only BFMatcher as LSH (FLANN) for 1-bit descriptor isn't supported
class RadiusBruteForceMatcherNodeType : public NodeType
{
public:
	RadiusBruteForceMatcherNodeType()
		: _maxDistance(100.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_MaxDistance:
			_maxDistance = newValue.toFloat();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_MaxDistance: return _maxDistance;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& queryDesc = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& trainDesc = reader.readSocket(3).getArray();
		// Acquire output sockets
		Matches& mt = writer.acquireSocket(0).getMatches();

		// Validate inputs
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

		cv::BFMatcher matcher(normType);
		vector<vector<cv::DMatch>> radiusMatches;

		matcher.radiusMatch(queryDesc, trainDesc, radiusMatches, _maxDistance);

		// remove unmatched ones and pick only first matches
		vector<cv::DMatch> matches;

		for(const auto& rmatches : radiusMatches)
		{
			if(!rmatches.empty())
			{
				matches.push_back(rmatches[0]);
			}
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(const auto& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Matches found: %d", (int) mt.queryPoints.size()));
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
			{ EPropertyType::Double, "Max distance", "min:0.0, decimals:2" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Finds the training descriptors not farther than the specified distance.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}
private:
	enum EPropertyID
	{
		ID_MaxDistance
	};

	float _maxDistance;
};

REGISTER_NODE("Features/Radius BForce Matcher", RadiusBruteForceMatcherNodeType);
REGISTER_NODE("Features/BForce Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/ANN Matcher", AproximateNearestNeighborMatcherNodeType)
