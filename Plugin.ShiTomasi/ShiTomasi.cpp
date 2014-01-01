#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"

#include <opencv2/imgproc/imgproc.hpp>

class ShiTomasiCornerDetectorNodeType : public NodeType
{
public:
	ShiTomasiCornerDetectorNodeType()
		: _maxCorners(100)
		, _qualityLevel(0.01)
		, _minDistance(10)
	{
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::MaxCorners:
			_maxCorners = newValue.toInt();
			return true;
		case pid::QualityLevel:
			_qualityLevel = newValue.toDouble();
			return true;
		case pid::MinDistance:
			_minDistance = newValue.toDouble();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::MaxCorners: return _maxCorners;
		case pid::QualityLevel: return _qualityLevel;
		case pid::MinDistance: return _minDistance;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		std::vector<cv::Point2f> corners;
		int blockSize = 3;
		bool useHarrisDetector = false;
		double k = 0.04;

		cv::goodFeaturesToTrack(src, corners, _maxCorners, _qualityLevel, 
			_minDistance, cv::noArray(), blockSize, useHarrisDetector, k);

		for(const auto& corner : corners)
			kp.kpoints.emplace_back(corner, 8.0f);
		kp.image = src;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Corners detected: %d", (int) kp.kpoints.size()));
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
			{ EPropertyType::Integer, "Max corners", "min:0" },
			{ EPropertyType::Double, "Quality level of octaves", "min:0" },
			{ EPropertyType::Double, "Minimum distance", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Shi-Tomasi corner detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		MaxCorners,
		QualityLevel,
		MinDistance
	};

	int _maxCorners;
	double _qualityLevel;
	double _minDistance;
};

extern "C" K_DECL_EXPORT int logicVersion()
{
	return LOGIC_VERSION;
}

extern "C" K_DECL_EXPORT int pluginVersion()
{
	return 1;
}

extern "C" K_DECL_EXPORT void registerPlugin(NodeSystem* nodeSystem)
{
	typedef DefaultNodeFactory<ShiTomasiCornerDetectorNodeType> ShiTomasiFactory;
	nodeSystem->registerNodeType("Features/Shi-Tomasi corner detector", 
		std::unique_ptr<NodeFactory>(new ShiTomasiFactory()));
}