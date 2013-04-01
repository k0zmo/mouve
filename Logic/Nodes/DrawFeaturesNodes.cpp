#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

class DrawKeypointsNodeType : public NodeType
{
public:
	DrawKeypointsNodeType()
		: _richKeypoints(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_RichKeypoints:
			_richKeypoints = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_RichKeypoints: return _richKeypoints;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints = reader.readSocket(1).getKeypoints();
		cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

		if(keypoints.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::drawKeypoints(imageSrc, keypoints, imageDst, 
			cv::Scalar(0, 0, 255), _richKeypoints
				? cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS
				: cv::DrawMatchesFlags::DEFAULT);

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
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Boolean, "Rich keypoints", "" },
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
		ID_RichKeypoints
	};

	bool _richKeypoints;
};

class DrawMatchesNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& image1 = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints1 = reader.readSocket(1).getKeypoints();
		const cv::Mat& image2 = reader.readSocket(2).getImage();
		const cv::KeyPoints& keypoints2 = reader.readSocket(3).getKeypoints();
		const cv::DMatches& matches = reader.readSocket(4).getMatches();

		cv::Mat& imageMatches = writer.acquireSocket(0).getImageRgb();

		if(keypoints1.empty() || keypoints2.empty() || !image1.data || !image2.data || matches.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::drawMatches(image1, keypoints1, image2, keypoints2,
			matches, imageMatches, cv::Scalar(0, 255, 0), cv::Scalar::all(-1),
			std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image1", "1st Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints1", "1st Keypoints", "" },
			{ ENodeFlowDataType::Image, "image2", "2nd Image", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints2", "2nd Keypoints", "" },
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DrawHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& img_object = reader.readSocket(0).getImage();
		const cv::Mat& img_scene = reader.readSocket(1).getImage();
		const cv::Mat& homography = reader.readSocket(2).getArray();

		cv::Mat& img_matches = writer.acquireSocket(0).getImageRgb();

		if(img_scene.empty() || img_object.empty() || homography.empty())
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Point2f> obj_corners(4);
		obj_corners[0] = cvPoint(0,0);
		obj_corners[1] = cvPoint(img_object.cols, 0);
		obj_corners[2] = cvPoint(img_object.cols, img_object.rows);
		obj_corners[3] = cvPoint(0, img_object.rows);

		std::vector<cv::Point2f> scene_corners(4);

		cv::perspectiveTransform(obj_corners, scene_corners, homography);

		//img_matches = img_scene.clone();
		cvtColor(img_scene, img_matches, CV_GRAY2BGR);

		cv::line(img_matches, scene_corners[0], scene_corners[1], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[1], scene_corners[2], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[2], scene_corners[3], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[3], scene_corners[0], cv::Scalar(255, 0, 0), 4);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Object image", "" },
			{ ENodeFlowDataType::Image, "source", "Base image", "" },
			{ ENodeFlowDataType::Array, "source", "Homography", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/Draw homography", DrawHomographyNodeType)
REGISTER_NODE("Features/Draw matches", DrawMatchesNodeType)
REGISTER_NODE("Features/Draw keypoints", DrawKeypointsNodeType)