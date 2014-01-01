#include "NodeType.h"
#include "NodeFactory.h"

#include <fstream>
#include <opencv2/calib3d/calib3d.hpp>

class EstimateHomographyNodeType : public NodeType
{
public:
	EstimateHomographyNodeType()
		: _reprojThreshold(3.0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ReprojThreshold:
			_reprojThreshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ReprojThreshold: return _reprojThreshold;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const Matches& mt = reader.readSocket(0).getMatches();
		// Acquire output sockets
		cv::Mat& H = writer.acquireSocket(0).getArray();
		Matches& outMt = writer.acquireSocket(1).getMatches();

		// Validate inputs
		if(mt.queryPoints.empty() || mt.trainPoints.empty()
		|| mt.queryImage.empty() || mt.trainImage.empty())
			return ExecutionStatus(EStatus::Ok);

		if(mt.queryPoints.size() != mt.trainPoints.size())
			return ExecutionStatus(EStatus::Error, 
				"Points from one images doesn't correspond to key points in another one");

		// Do stuff
		outMt.queryImage = mt.queryImage;
		outMt.trainImage = mt.trainImage;
		outMt.queryPoints.clear();
		outMt.trainPoints.clear();
		size_t kpSize = mt.queryPoints.size();

		if(mt.queryPoints.size() < 4)
		{
			H = cv::Mat::ones(3, 3, CV_64F);
			return ExecutionStatus(EStatus::Ok, "Not enough matches for homography estimation");
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

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Inliers: %d\nOutliers: %d\nPercent of correct matches: %f%%",
				(int) outMt.queryPoints.size(), 
				(int) (mt.queryPoints.size() - outMt.queryPoints.size()), 
				(double) outMt.queryPoints.size() / mt.queryPoints.size() * 100.0)
			);
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

		nodeConfig.description = "Finds a perspective transformation between two planes.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		ReprojThreshold
	};

	double _reprojThreshold;
};

class KnownHomographyInliersNodeType : public NodeType
{
public:
	KnownHomographyInliersNodeType()
		: _homographyPath()
		, _reprojThreshold(3.0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::HomographyPath:
			_homographyPath = newValue.toFilepath();
			return true;
		case pid::ReprojThreshold:
			_reprojThreshold = newValue.toDouble();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::HomographyPath: return _homographyPath;
		case pid::ReprojThreshold: return _reprojThreshold;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const Matches& mt = reader.readSocket(0).getMatches();
		// Acquire output sockets
		cv::Mat& H = writer.acquireSocket(0).getArray();
		Matches& outMt = writer.acquireSocket(1).getMatches();

		// Validate inputs
		if(mt.queryPoints.empty() || mt.trainPoints.empty()
		|| mt.queryImage.empty() || mt.trainImage.empty())
			return ExecutionStatus(EStatus::Ok);

		if(mt.queryPoints.size() != mt.trainPoints.size())
			return ExecutionStatus(EStatus::Error, 
			"Points from one images doesn't correspond to key points in another one");

		// Do stuff - Load real homography 
		std::ifstream homographyFile(_homographyPath.data(), std::ios::in);
		if(!homographyFile.is_open())
			return ExecutionStatus(EStatus::Error, formatMessage("Can't load %s\n", _homographyPath.data().c_str()));

		cv::Mat homography(3, 3, CV_32F);
		homographyFile >> homography.at<float>(0, 0) >> homography.at<float>(0, 1) >> homography.at<float>(0, 2);
		homographyFile >> homography.at<float>(1, 0) >> homography.at<float>(1, 1) >> homography.at<float>(1, 2);
		homographyFile >> homography.at<float>(2, 0) >> homography.at<float>(2, 1) >> homography.at<float>(2, 2);

		// Normalize if h[2][2] isnt close to 1.0 and 0.0
		float h22 = homography.at<float>(2,2);
		if(std::fabs(h22) > 1e-5 && std::fabs(h22 - 1.0) > 1e-5)
		{
			h22 = 1.0f / h22;
			homography.at<float>(0,0) *= h22; homography.at<float>(0,1) *= h22; homography.at<float>(0,2) *= h22;
			homography.at<float>(1,0) *= h22; homography.at<float>(1,1) *= h22; homography.at<float>(1,2) *= h22;
			homography.at<float>(2,0) *= h22; homography.at<float>(2,1) *= h22; homography.at<float>(2,2) *= h22;
		}

		outMt.queryImage = mt.queryImage;
		outMt.trainImage = mt.trainImage;
		outMt.queryPoints.clear();
		outMt.trainPoints.clear();

		for(size_t i = 0; i < mt.queryPoints.size(); ++i)
		{
			const auto& pt1 = mt.queryPoints[i];
			const auto& pt2 = mt.trainPoints[i];

			float invW = 1.0f / (homography.at<float>(2,0)*pt1.x + homography.at<float>(2,1)*pt1.y + homography.at<float>(2,2));
			float xt = (homography.at<float>(0,0)*pt1.x + homography.at<float>(0,1)*pt1.y + homography.at<float>(0,2)) * invW;
			float yt = (homography.at<float>(1,0)*pt1.x + homography.at<float>(1,1)*pt1.y + homography.at<float>(1,2)) * invW;
			
			float xdist = xt - pt2.x;
			float ydist = yt - pt2.y;
			float dist = std::sqrt(xdist*xdist + ydist*ydist);

			if(dist <= _reprojThreshold)
			{
				outMt.queryPoints.push_back(pt1);
				outMt.trainPoints.push_back(pt2);
			}
		}

		H = homography;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Inliers: %d\nOutliers: %d\nPercent of correct matches: %f%%",
				(int) outMt.queryPoints.size(), 
				(int) (mt.queryPoints.size() - outMt.queryPoints.size()), 
				(double) outMt.queryPoints.size() / mt.queryPoints.size() * 100.0)
			);
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
			{ EPropertyType::Filepath, "Homography path", "" },
			{ EPropertyType::Double, "Reprojection error threshold", "min:1.0, max:50.0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Rejects outliers using known homography.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		HomographyPath,
		ReprojThreshold
	};

	Filepath _homographyPath;
	double _reprojThreshold;
};

REGISTER_NODE("Features/Known homography inliers", KnownHomographyInliersNodeType)
REGISTER_NODE("Features/Estimate homography", EstimateHomographyNodeType)