#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

enum EColor
{
	Color_AllRandom,
	Color_Red,
	Color_Green,
	Color_Blue
};

cv::Scalar getColor(EColor color)
{
	switch(color)
	{
	// Order is BGR
	case Color_AllRandom: return cv::Scalar::all(-1);
	case Color_Red: return cv::Scalar(0, 0, 255);
	case Color_Green: return cv::Scalar(0, 255, 0);
	case Color_Blue: return cv::Scalar(255, 0, 0);
	}
	return cv::Scalar::all(-1);
}

EColor getColor(const cv::Scalar& scalar)
{
	if(scalar == cv::Scalar::all(-1))
		return Color_AllRandom;
	else if(scalar == cv::Scalar(0, 0, 255))
		return Color_Red;
	else if(scalar == cv::Scalar(0, 255, 0))
		return Color_Green;
	else if(scalar == cv::Scalar(255, 0, 0))
		return Color_Blue;
	else
		return Color_AllRandom;
}

class DrawSomethingLinesNodeType : public NodeType
{
public:
	DrawSomethingLinesNodeType()
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
		// inputs
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::Mat& circles = reader.readSocket(1).getArray();
		// ouputs
		cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(imageSrc.empty())
			return ExecutionStatus(EStatus::Ok);

		cvtColor(imageSrc, imageDst, CV_GRAY2BGR);

		return executeImpl(circles, imageSrc, imageDst);
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
			{ EPropertyType::Enum, "Line color", "item: Random, item: Red, item: Green, item: Blue" },
			{ EPropertyType::Integer, "Line thickness", "step:1, min:1, max:5" },
			{ EPropertyType::Enum, "Line type", "item: 4-connected, item: 8-connected, item: AA" },
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

protected:
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

	virtual ExecutionStatus executeImpl(const cv::Mat& objects, 
		const cv::Mat& imageSrc, cv::Mat& imageDest) = 0;
};

class DrawLinesNodeType : public DrawSomethingLinesNodeType
{
public:
	ExecutionStatus executeImpl(const cv::Mat& objects, 
		const cv::Mat& imageSrc, cv::Mat& imageDest) override
	{
		cv::RNG& rng = cv::theRNG();
		bool isRandColor = _color == cv::Scalar::all(-1);

		for(int lineIdx = 0; lineIdx < objects.rows; ++lineIdx)
		{
			float rho = objects.at<float>(lineIdx, 0);
			float theta = objects.at<float>(lineIdx, 1);
			double cos_t = cos(theta);
			double sin_t = sin(theta);
			double x0 = rho*cos_t;
			double y0 = rho*sin_t;
			double alpha = sqrt(imageSrc.cols*imageSrc.cols + imageSrc.rows*imageSrc.rows);

			cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
			cv::Point pt1(cvRound(x0 + alpha*(-sin_t)), cvRound(y0 + alpha*cos_t));
			cv::Point pt2(cvRound(x0 - alpha*(-sin_t)), cvRound(y0 - alpha*cos_t));
			cv::line(imageDest, pt1, pt2, color, _thickness, int(_type));
		}

		return ExecutionStatus(EStatus::Ok);
	}
};

class DrawCirclesNodeType : public DrawSomethingLinesNodeType
{
public:
	ExecutionStatus executeImpl(const cv::Mat& objects,
		const cv::Mat& /*imageSrc*/, cv::Mat& imageDest) override
	{
		cv::RNG& rng = cv::theRNG();
		bool isRandColor = _color == cv::Scalar::all(-1);

		for(int circleIdx = 0; circleIdx < objects.cols; ++circleIdx)
		{
			cv::Vec3f circle = objects.at<cv::Vec3f>(circleIdx);
			cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
			int radius = cvRound(circle[2]);
			cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
			// draw the circle center
			cv::circle(imageDest, center, 3, color, _thickness, int(_type));
			// draw the circle outline
			cv::circle(imageDest, center, radius, color, _thickness, int(_type));
		}

		return ExecutionStatus(EStatus::Ok);
	}
};

class DrawKeypointsNodeType : public NodeType
{
public:
	DrawKeypointsNodeType()
		: _richKeypoints(false)
		, _color(cv::Scalar::all(-1))
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_RichKeypoints:
			_richKeypoints = newValue.toBool();
			return true;
		case ID_Color:
			_color = getColor(EColor(newValue.toUInt()));
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_RichKeypoints: return _richKeypoints;
		case ID_Color: return getColor(_color);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();
		// outputs
		cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		int drawFlags = _richKeypoints
			? cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS
			: cv::DrawMatchesFlags::DEFAULT;
		cv::drawKeypoints(kp.image, kp.kpoints, imageDst, _color, drawFlags);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Keypoints color", "item: Random, item: Red, item: Green, item: Blue" },
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
		ID_Color,
		ID_RichKeypoints
	};

	bool _richKeypoints;
	cv::Scalar _color;
};

class DrawMatchesNodeType : public NodeType
{
public:
	DrawMatchesNodeType()
		: _color(cv::Scalar::all(-1))
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Color:
			_color = getColor(EColor(newValue.toUInt()));
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Color: return getColor(_color);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const Matches& mt = reader.readSocket(0).getMatches();
		// outputs
		cv::Mat& imageMatches = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(mt.queryImage.empty() || mt.trainImage.empty())
			return ExecutionStatus(EStatus::Ok);

		if(mt.queryPoints.size() != mt.trainPoints.size())
			return ExecutionStatus(EStatus::Error, 
				"Points from one images doesn't correspond to key points in another one");

		imageMatches = drawMatchesOnImage(mt);
		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Keypoints color", "item: Random, item: Red, item: Green, item: Blue" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	cv::Mat drawMatchesOnImage(const Matches& mt)
	{
		// mt.queryImage.type() == mt.trainImage.type() for now
		int cols = mt.queryImage.cols + mt.trainImage.cols;
		int rows = std::max(mt.queryImage.rows, mt.trainImage.rows);
		cv::Mat matDraw(rows, cols, mt.queryImage.type(), cv::Scalar(0));

		cv::Rect roi1(0, 0, mt.queryImage.cols, mt.queryImage.rows);
		cv::Rect roi2(mt.queryImage.cols, 0, mt.trainImage.cols, mt.trainImage.rows);

		mt.queryImage.copyTo(matDraw(roi1));
		mt.trainImage.copyTo(matDraw(roi2));
		if(matDraw.channels() == 1)
			cvtColor(matDraw, matDraw, cv::COLOR_GRAY2BGR);

		cv::Mat matDraw1 = matDraw(roi1);
		cv::Mat matDraw2 = matDraw(roi2);
		cv::RNG& rng = cv::theRNG();
		bool isRandColor = _color == cv::Scalar::all(-1);

		for(size_t i = 0; i < mt.queryPoints.size(); ++i)
		{
			auto&& kp1 = mt.queryPoints[i];
			auto&& kp2 = mt.trainPoints[i];

			cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
			
			auto drawKeypoint = [](cv::Mat& matDraw, const cv::Point2f& pt, 
				const cv::Scalar& color)
			{
				int x = cvRound(pt.x);
				int y = cvRound(pt.y);
				int s = 10;

				cv::circle(matDraw, cv::Point(x, y), s, color, 1, CV_AA);
			};

			drawKeypoint(matDraw1, kp1, color);
			drawKeypoint(matDraw2, kp2, color);

			int x1 = cvRound(kp1.x) + roi1.tl().x;
			int y1 = cvRound(kp1.y) + roi1.tl().y;
			int x2 = cvRound(kp2.x) + roi2.tl().x;
			int y2 = cvRound(kp2.y) + roi2.tl().y;
			cv::line(matDraw, cv::Point(x1, y1), cv::Point(x2, y2), color, 1, CV_AA);
		}

		return matDraw;
	}

private:
	enum EPropertyID
	{
		ID_Color
	};

	cv::Scalar _color;
};

class DrawHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		//inputs
		const cv::Mat& homography = reader.readSocket(0).getArray();
		// in fact we need only one image and size of another but this solution is way cleaner
		const Matches& mt = reader.readSocket(1).getMatches();
		// outputs
		cv::Mat& img_matches = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(mt.queryImage.empty() || mt.trainImage.empty() || homography.empty())
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Point2f> obj_corners(4);
		obj_corners[0] = cvPoint(0,0);
		obj_corners[1] = cvPoint(mt.queryImage.cols, 0);
		obj_corners[2] = cvPoint(mt.queryImage.cols, mt.queryImage.rows);
		obj_corners[3] = cvPoint(0, mt.queryImage.rows);

		std::vector<cv::Point2f> scene_corners(4);

		cv::perspectiveTransform(obj_corners, scene_corners, homography);

		//img_matches = img_scene.clone();
		cvtColor(mt.trainImage, img_matches, CV_GRAY2BGR);

		cv::line(img_matches, scene_corners[0], scene_corners[1], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[1], scene_corners[2], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[2], scene_corners[3], cv::Scalar(255, 0, 0), 4);
		cv::line(img_matches, scene_corners[3], scene_corners[0], cv::Scalar(255, 0, 0), 4);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Array, "source", "Homography", "" },
			{ ENodeFlowDataType::Matches, "source", "Matches", "" },
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
REGISTER_NODE("Draw features/Draw circles", DrawCirclesNodeType)
REGISTER_NODE("Draw features/Draw lines", DrawLinesNodeType)