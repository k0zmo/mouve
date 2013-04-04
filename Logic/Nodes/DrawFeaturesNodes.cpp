#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

enum EColor
{
	Color_Red,
	Color_Green,
	Color_Blue,
	Color_Invalid
};

cv::Scalar getColor(EColor color)
{
	switch(color)
	{
	// Order is BGR
	case Color_Red: return cv::Scalar(0, 0, 255);
	case Color_Green: return cv::Scalar(0, 255, 0);
	case Color_Blue: return cv::Scalar(255, 0, 0);
	}
	return cv::Scalar(255, 255, 255);
}

EColor getColor(cv::Scalar scalar)
{
	if(scalar == cv::Scalar(0, 0, 255))
		return Color_Red;
	else if(scalar == cv::Scalar(0, 255, 0))
		return Color_Green;
	else if(scalar == cv::Scalar(255, 0, 0))
		return Color_Blue;
	return Color_Invalid;
}

class DrawLinesNodeType : public NodeType
{
public:
	DrawLinesNodeType()
		: _color(cv::Scalar(255, 0, 0))
		, _thickness(2)
		, _type(ELineType::Line_AA)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_LineColor:
			_color = getColor(EColor(newValue.toUInt()));
			return true;
		case ID_LineThickness:
			_thickness = newValue.toUInt();
			return true;
		case ID_LineType:
			_type = lineType(newValue.toUInt());
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_LineColor: return getColor(_color);
		case ID_LineThickness: return _thickness;
		case ID_LineType: return lineTypeIndex(_type);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::Mat& lines = reader.readSocket(1).getArray();
		cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

		if(imageSrc.empty())
			return ExecutionStatus(EStatus::Ok);

		cvtColor(imageSrc, imageDst, CV_GRAY2BGR);

		for(int lineIdx = 0; lineIdx < lines.rows; ++lineIdx)
		{
			float rho = lines.at<float>(lineIdx, 0);
			float theta = lines.at<float>(lineIdx, 1);
			double cos_t = cos(theta);
			double sin_t = sin(theta);
			double x0 = rho*cos_t;
			double y0 = rho*sin_t;
			double alpha = sqrt(imageSrc.cols*imageSrc.cols + imageSrc.rows*imageSrc.rows);

			cv::Point pt1(cvRound(x0 + alpha*(-sin_t)), cvRound(y0 + alpha*cos_t));
			cv::Point pt2(cvRound(x0 - alpha*(-sin_t)), cvRound(y0 - alpha*cos_t));
			cv::line(imageDst, pt1, pt2, _color, _thickness, int(_type));
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Image", "" },
			{ ENodeFlowDataType::Array, "lines", "Lines", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Line color", "item: Red, item: Green, item: Blue" },
			{ EPropertyType::Integer, "Line thickness", "step:1, min:1, max:5" },
			{ EPropertyType::Enum, "Line type", "item: 4-connected, item: 8-connected, item: AA" },
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
		ID_LineColor,
		ID_LineThickness,
		ID_LineType
	};

	enum class ELineType
	{
		Line_4Connected = 4,
		Line_8Connected = 8,
		Line_AA         = CV_AA
	};

	cv::Scalar _color;
	int _thickness;
	ELineType _type;

private:
	ELineType lineType(int lineTypeIndex) const
	{
		switch(lineTypeIndex)
		{
		case 0: return ELineType::Line_4Connected;
		case 1: return ELineType::Line_8Connected;
		case 2: return ELineType::Line_AA;
		}
		return ELineType::Line_4Connected;
	}

	int lineTypeIndex(ELineType type) const
	{
		switch(type)
		{
		case ELineType::Line_4Connected: return 0;
		case ELineType::Line_8Connected: return 1;
		case ELineType::Line_AA: return 2;
		}
		return 0;
	}
};

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

REGISTER_NODE("Draw features/Draw homography", DrawHomographyNodeType)
REGISTER_NODE("Draw features/Draw matches", DrawMatchesNodeType)
REGISTER_NODE("Draw features/Draw keypoints", DrawKeypointsNodeType)
REGISTER_NODE("Draw features/Draw lines", DrawLinesNodeType)