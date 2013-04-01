#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/calib3d/calib3d.hpp>

class EstimateHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::KeyPoints& keypoints1 = reader.readSocket(0).getKeypoints();
		const cv::KeyPoints& keypoints2 = reader.readSocket(1).getKeypoints();
		const cv::DMatches& matches = reader.readSocket(2).getMatches();
		cv::Mat& H = writer.acquireSocket(0).getArray();

		if(keypoints1.empty() || keypoints2.empty() || matches.size() < 4)
			return ExecutionStatus(EStatus::Ok);

		//-- Localize the object
		std::vector<cv::Point2f> obj;
		std::vector<cv::Point2f> scene;

		for(auto& match : matches)
		{
			obj.push_back(keypoints1[match.queryIdx].pt);
			scene.push_back(keypoints2[match.trainIdx].pt);
		}

		H = cv::findHomography(obj, scene, cv::RANSAC);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints1", "1st Keypoints", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints1", "2nd Keypoints", "" },
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "output", "Homography", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/Estimate homography", EstimateHomographyNodeType)