#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/nonfree/features2d.hpp>

class FastFeatureDetector : public NodeType
{
public:
	FastFeatureDetector()
		: _threshold(10)
		, _type(cv::FastFeatureDetector::TYPE_9_16)
		, _nonmaxSupression(true)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case 0:
			_threshold = newValue.toInt();
			return true;
		case 1:
			_nonmaxSupression = newValue.toBool();
			return true;
		case 2:
			_type = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case 0: return _threshold;
		case 1: return _nonmaxSupression;
		case 2: return _type;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::FASTX(src, kp.kpoints, _threshold, _nonmaxSupression, _type);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
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
			{ EPropertyType::Integer, "Threshold", "min:0, max:255" },
			{ EPropertyType::Boolean, "Nonmax supression", "" },
			{ EPropertyType::Enum, "Neighborhoods type", "item: 5_8, item: 7_12, item: 9_16" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Detects corners using the FAST algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int _threshold;
	int _type;
	bool _nonmaxSupression;
};

class MserSalientRegionDetectorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& img = writer.acquireSocket(0).getImageRgb();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cvtColor(src, img, CV_GRAY2BGR);

		cv::MserFeatureDetector mser;
		vector<vector<cv::Point>> msers;
		mser(src, msers);

		for(int i = (int)msers.size()-1; i >= 0; i--)
		{
			// fit ellipse
			cv::RotatedRect box = cv::fitEllipse(msers[i]);
			box.angle = (float)CV_PI/2 - box.angle;
			cv::ellipse(img, box, cv::Scalar(196,255,255), 1, CV_AA);
		}		

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Maximally stable extremal region extractor.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class StarFeatureDetectorNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::StarFeatureDetector star;
		star(src, kp.kpoints);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
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

		nodeConfig.description = "CenSurE keypoint detector.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Features/STAR", StarFeatureDetectorNodeType)
REGISTER_NODE("Features/MSER", MserSalientRegionDetectorNodeType)
REGISTER_NODE("Features/FAST", FastFeatureDetector)