#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

class SurfFeatureDetectorNodeType : public NodeType
{
public:
	SurfFeatureDetectorNodeType()
		: _hessianThreshold(400.0)
		, _nOctaves(4)
		, _nOctaveLayers(2)
		, _extended(true)
		, _upright(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_HessianThreshold:
			_hessianThreshold = newValue.toDouble();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case ID_NumOctaveLayers:
			_nOctaveLayers = newValue.toInt();
			return true;
		case ID_Extended:
			_extended = newValue.toBool();
			return true;
		case ID_Upright:
			_upright = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_HessianThreshold: return _hessianThreshold;
		case ID_NumOctaves: return _nOctaves;
		case ID_NumOctaveLayers: return _nOctaveLayers;
		case ID_Extended: return _extended;
		case ID_Upright: return _upright;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::KeyPoints& keypoints = writer.acquireSocket(0).getKeypoints();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves,
			_nOctaveLayers, _extended, _upright);
		detector.detect(src, keypoints);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "min:1" },
			{ EPropertyType::Integer, "Number of octave layers", "min:1" },
			{ EPropertyType::Boolean, "Extended", "" },
			{ EPropertyType::Boolean, "Upright", "" },
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
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumOctaveLayers,
		ID_Extended,
		ID_Upright
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nOctaveLayers;
	bool _extended;
	bool _upright;
};

class SurfFeatureExtractorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& imageSrc = reader.readSocket(0).getImage();
		const cv::KeyPoints& keypoints = reader.readSocket(1).getKeypoints();

		if(keypoints.empty() || imageSrc.cols == 0 || imageSrc.rows == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::Mat& outDescriptors = writer.acquireSocket(0).getArray();
		cv::KeyPoints& outKeypoints = writer.acquireSocket(1).getKeypoints();

		outKeypoints = keypoints;

		cv::SurfDescriptorExtractor extractor;
		extractor.compute(imageSrc, outKeypoints, outDescriptors);

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
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
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

		if(qFuzzyCompare(0.0, _threshold))
		{

			cv::FlannBasedMatcher matcher;
			matcher.match(queryDescriptors, trainDescriptors, matches);
		}
		else
		{
			cv::DMatches tmpMatches;
			cv::FlannBasedMatcher matcher;
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

			double th = _threshold * min_dist;

			for(int i = 0; i < queryDescriptors.rows; ++i)
			{
				if(tmpMatches[i].distance < th)
					matches.push_back(tmpMatches[i]);
			}
		}

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

class EstimateHomographyNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::KeyPoints& keypoints1 = reader.readSocket(0).getKeypoints();
		const cv::KeyPoints& keypoints2 = reader.readSocket(1).getKeypoints();
		const cv::DMatches& matches = reader.readSocket(2).getMatches();
		cv::Mat& H = writer.acquireSocket(0).getArray();

		if(keypoints1.empty() || keypoints2.empty() || matches.empty())
			return ExecutionStatus(EStatus::Ok);

		//-- Localize the object
		std::vector<cv::Point2f> obj;
		std::vector<cv::Point2f> scene;

		for( int i = 0; i < matches.size(); i++ )
		{
			obj.push_back(keypoints1[matches[i].queryIdx].pt);
			scene.push_back(keypoints2[matches[i].trainIdx].pt);
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

		if(!img_scene.data || !img_object.data)
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
REGISTER_NODE("Features/Estimate homography", EstimateHomographyNodeType)
REGISTER_NODE("Features/NN Matcher", NearestNeighborMatcherNodeType)
REGISTER_NODE("Features/SURF Extractor", SurfFeatureExtractorNodeType)
REGISTER_NODE("Features/SURF Detector", SurfFeatureDetectorNodeType)