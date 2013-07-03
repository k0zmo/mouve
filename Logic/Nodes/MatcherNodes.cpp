#include "Prerequisites.h"
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

	for(auto&& knMatch : knMatches)
	{
		if(knMatch.size() != 2)
			continue;

		auto&& best = knMatch[0];
		auto&& good = knMatch[1];

		if(best.distance <= distanceRatioThreshold * good.distance)
			positiveMatches.push_back(best);
	}

	return positiveMatches;
}

static vector<cv::DMatch> symmetryTest(const vector<cv::DMatch>& matches1to2,
									   const vector<cv::DMatch>& matches2to1)
{
	vector<cv::DMatch> bothMatches;

	for(auto&& match1to2 : matches1to2)
	{
		for(auto&& match2to1 : matches2to1)
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

		nodeConfig.description = "";
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
		// inputs
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const cv::Mat& query = reader.readSocket(1).getArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const cv::Mat& train = reader.readSocket(3).getArray();
		// outputs
		Matches& mt = writer.acquireSocket(0).getMatches();

		// validate inputs
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

		vector<cv::DMatch> matches;
		size_t initialMatches = 0;

		if(_symmetryTest)
		{
			vector<vector<cv::DMatch>> knMatches1to2 = knnMatch(query, train);
			vector<vector<cv::DMatch>> knMatches2to1 = knnMatch(train, query);
			initialMatches = knMatches1to2.size();

			auto matches1to2 = distanceRatioTest(knMatches1to2, _distanceRatio);
			auto matches2to1 = distanceRatioTest(knMatches2to1, _distanceRatio);
			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			vector<vector<cv::DMatch>> knMatches = knnMatch(query, train);
			initialMatches = knMatches.size();

			matches = distanceRatioTest(knMatches, _distanceRatio);
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(auto&& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Initial matches found: %d\nFinal matches found: %d",
			(int) initialMatches, (int) mt.queryPoints.size()));
	}

private:
	vector<vector<cv::DMatch>> knnMatch(const cv::Mat& query, const cv::Mat& train)
	{
		vector<vector<cv::DMatch>> knMatches;

		if(query.type() == CV_32F)
		{
			// for each descriptors in query array
			for(int queryIdx = 0; queryIdx < query.rows; ++queryIdx)
			{
				const float* query_row = query.ptr<float>(queryIdx);

				float bestDistance1 = std::numeric_limits<float>::max();
				float bestDistance2 = std::numeric_limits<float>::max();
				int bestTrainIdx1 = -1;
				int bestTrainIdx2 = -1;

				// for each descriptors in train array
				for(int trainIdx = 0; trainIdx < train.rows; ++trainIdx)
				{
					const float* train_row = train.ptr<float>(trainIdx);

					cv::L2<float> op;
					cv::L2<float>::ResultType dist = op(train_row, query_row, train.cols);

					if(dist < bestDistance1)
					{
						bestDistance2 = bestDistance1;
						bestTrainIdx2 = bestTrainIdx1;
						bestDistance1 = dist;
						bestTrainIdx1 = trainIdx;
					}
					else if(dist < bestDistance2)
					{
						bestDistance2 = dist;
						bestTrainIdx2 = trainIdx;
					}
				}

				vector<cv::DMatch> knMatch;
				knMatch.emplace_back(cv::DMatch(queryIdx, bestTrainIdx1, bestDistance1));
				knMatch.emplace_back(cv::DMatch(queryIdx, bestTrainIdx2, bestDistance2));
				knMatches.emplace_back(knMatch);
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
				int bestTrainIdx2 = -1;

				// for each descriptors in train array
				for(int trainIdx = 0; trainIdx < train.rows; ++trainIdx)
				{
					const uchar* train_row = train.ptr<uchar>(trainIdx);

					cv::Hamming op;
					cv::Hamming::ResultType dist = op(train_row, query_row, train.cols);

					if(dist < bestDistance1)
					{
						bestDistance2 = bestDistance1;
						bestTrainIdx2 = bestTrainIdx1;
						bestDistance1 = dist;
						bestTrainIdx1 = trainIdx;
					}
					else if(dist < bestDistance2)
					{
						bestDistance2 = dist;
						bestTrainIdx2 = trainIdx;
					}
				}

				vector<cv::DMatch> knMatch;
				knMatch.emplace_back(cv::DMatch(queryIdx, bestTrainIdx1, bestDistance1));
				knMatch.emplace_back(cv::DMatch(queryIdx, bestTrainIdx2, bestDistance2));
				knMatches.emplace_back(knMatch);
			}
		}

		return knMatches;
	}
};

class AproximateNearestNeighborMatcherNodeType : public MatcherNodeType
{
public:
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
		vector<cv::DMatch> matches;
		size_t initialMatches = 0;

		if(_symmetryTest)
		{
			vector<vector<cv::DMatch>> knMatches1to2, knMatches2to1;

			matcher.knnMatch(queryDesc, trainDesc, knMatches1to2, 2);
			matcher.knnMatch(trainDesc, queryDesc, knMatches2to1, 2);
			initialMatches = knMatches1to2.size();

			auto matches1to2 = distanceRatioTest(knMatches1to2, _distanceRatio);
			auto matches2to1 = distanceRatioTest(knMatches2to1, _distanceRatio);
			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			vector<vector<cv::DMatch>> knMatches;
			matcher.knnMatch(queryDesc, trainDesc, knMatches, 2);
			initialMatches = knMatches.size();

			matches = distanceRatioTest(knMatches, _distanceRatio);
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(auto&& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Initial matches found: %d\nFinal Matches found: %d",
			(int) initialMatches, (int) mt.queryPoints.size()));
	}
};

REGISTER_NODE("Features/BForce Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/ANN Matcher", AproximateNearestNeighborMatcherNodeType)
