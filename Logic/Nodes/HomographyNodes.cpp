#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/calib3d/calib3d.hpp>

class EstimateHomographyNodeType : public NodeType
{
public:
	EstimateHomographyNodeType()
		: _reprojThreshold(3.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_ReprojThreshold:
			_reprojThreshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_ReprojThreshold: return _reprojThreshold;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const Matches& mt = reader.readSocket(0).getMatches();
		// outputs
		cv::Mat& H = writer.acquireSocket(0).getArray();
		Matches& outMt = writer.acquireSocket(1).getMatches();

		// validate inputs
		if(mt.queryPoints.empty() || mt.trainPoints.empty()
		|| mt.queryImage.empty() || mt.trainImage.empty())
			return ExecutionStatus(EStatus::Ok);

		if(mt.queryPoints.size() != mt.trainPoints.size())
			return ExecutionStatus(EStatus::Error, 
				"Points from one images doesn't correspond to key points in another one");

		outMt.queryImage = mt.queryImage;
		outMt.trainImage = mt.trainImage;
		outMt.queryPoints.clear();
		outMt.trainPoints.clear();
		size_t kpSize = mt.queryPoints.size();

		if(mt.queryPoints.size() < 4)
		{
			H = cv::Mat::ones(3, 3, CV_64F);
			return ExecutionStatus(EStatus::Ok);
		}

		vector<uchar> inliersMask;
		cv::Mat homography = cv::findHomography(mt.queryPoints, mt.trainPoints, 
			CV_RANSAC, _reprojThreshold, inliersMask);
		int inliersCount = (int) std::count(begin(inliersMask), end(inliersMask), 1);
		if(inliersCount < 4)
			return ExecutionStatus(EStatus::Ok);

		vector<cv::Point2f> queryPoints(inliersCount), trainPoints(inliersCount);

		// Create vector of inliers only
		for(size_t inlier = 0, idx = 0; inlier < kpSize; ++inlier)
		{
			if(inliersMask[inlier] == 0)
				continue;

			queryPoints[idx] = mt.queryPoints[inlier];
			trainPoints[idx] = mt.trainPoints[inlier];
			++idx;
		}

		// Use only good points to find refined homography
		H = cv::findHomography(queryPoints, trainPoints, 0);

		// Reproject again
		vector<cv::Point2f> srcReprojected;
		cv::perspectiveTransform(trainPoints, srcReprojected, H.inv());
		kpSize = queryPoints.size();

		for (size_t i = 0; i < kpSize; i++)
		{
			cv::Point2f actual = queryPoints[i];
			cv::Point2f expect = srcReprojected[i];
			cv::Point2f v = actual - expect;
			float distanceSquared = v.dot(v);

			if (distanceSquared <= _reprojThreshold * _reprojThreshold)
			{
				outMt.queryPoints.emplace_back(queryPoints[i]);
				outMt.trainPoints.emplace_back(trainPoints[i]);
			}
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "output", "Homography", "" },
			{ ENodeFlowDataType::Matches, "inliers", "Inliers", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Reprojection error threshold", "min:1.0, max:50.0" },
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
		ID_ReprojThreshold
	};

	double _reprojThreshold;
};

REGISTER_NODE("Features/Estimate homography", EstimateHomographyNodeType)