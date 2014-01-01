#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"

#include "brisk.h"

class BriskFeatureDetectorNodeType : public NodeType
{
public:
	BriskFeatureDetectorNodeType()
		: _thresh(60)
		, _nOctaves(4)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold:
			_thresh = newValue.toInt();
			return true;
		case pid::NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold: return _thresh;
		case pid::NumOctaves: return _nOctaves;
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

		cv::BriskFeatureDetector brisk(_thresh, _nOctaves);
		brisk.detect(src, kp.kpoints);
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
			{ EPropertyType::Integer, "FAST/AGAST detection threshold score", "min:1" },
			{ EPropertyType::Integer, "Number of octaves", "min:0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Threshold,
		NumOctaves
	};

	int _thresh;
	int _nOctaves;
};

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
	BriskDescriptorExtractorNodeType()
		: _rotationInvariant(true)
		, _scaleInvariant(true)
		, _patternScale(1.0f)
		, _brisk(new cv::BriskDescriptorExtractor(_rotationInvariant, _scaleInvariant, _patternScale))
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		if(propId > underlying_cast(pid::PatternScale) || 
			propId < underlying_cast(pid::RotationInvariant))
			return false;

		switch(static_cast<pid>(propId))
		{
		case pid::RotationInvariant:
			_rotationInvariant = newValue.toBool();
			break;
		case pid::ScaleInvariant:
			_scaleInvariant = newValue.toBool();
			break;
		case pid::PatternScale:
			_patternScale  = newValue.toFloat();
			break;
		}

		// That's a bit cheating here - creating BRISK object takes time (approx. 200ms for PAL)
		// which if done per frame makes it slowish (more than SIFT actually)
		_brisk = std::unique_ptr<cv::BriskDescriptorExtractor>(new cv::BriskDescriptorExtractor(
			_rotationInvariant, _scaleInvariant, _patternScale));
		return true;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::RotationInvariant: return _rotationInvariant;
		case pid::ScaleInvariant: return _scaleInvariant;
		case pid::PatternScale: return _patternScale;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();

		// validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		// outputs
		KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
		outKp = kp;

		_brisk->compute(kp.image, outKp.kpoints, outDescriptors);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Boolean, "Rotation invariant", "" },
			{ EPropertyType::Boolean, "Scale invariant", "" },
			{ EPropertyType::Double, "Pattern scale", "min:0.0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		RotationInvariant,
		ScaleInvariant,
		PatternScale
	};

	bool _rotationInvariant;
	bool _scaleInvariant;
	float _patternScale;

	// Must be pointer since BriskDescriptorExtractor doesn't implement copy/move operator
	// and we want to cache _brisk object
	std::unique_ptr<cv::BriskDescriptorExtractor> _brisk;
};

class BriskNodeType : public NodeType
{
public:
	BriskNodeType()
		: _thresh(60)
		, _nOctaves(4)
		, _rotationInvariant(true)
		, _scaleInvariant(true)
		, _patternScale(1.0f)
		, _brisk(new cv::BriskDescriptorExtractor(_rotationInvariant, _scaleInvariant, _patternScale))
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold:
			_thresh = newValue.toInt();
			return true;
		case pid::NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case pid::RotationInvariant:
			_rotationInvariant = newValue.toBool();
			break;
		case pid::ScaleInvariant:
			_scaleInvariant = newValue.toBool();
			break;
		case pid::PatternScale:
			_patternScale  = newValue.toFloat();
			break;
		}

		// That's a bit cheating here - creating BRISK object takes time (approx. 200ms for PAL)
		// which if done per frame makes it slowish (more than SIFT actually)
		_brisk = std::unique_ptr<cv::BriskDescriptorExtractor>(new cv::BriskDescriptorExtractor(
			_rotationInvariant, _scaleInvariant, _patternScale));
		return true;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Threshold: return _thresh;
		case pid::NumOctaves: return _nOctaves;
		case pid::RotationInvariant: return _rotationInvariant;
		case pid::ScaleInvariant: return _scaleInvariant;
		case pid::PatternScale: return _patternScale;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// ouputs
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cv::BriskFeatureDetector _briskFeatureDetector(_thresh, _nOctaves);
		_briskFeatureDetector.detect(src, kp.kpoints);
		_brisk->compute(src, kp.kpoints, descriptors);
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
			{ ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "FAST/AGAST detection threshold score", "min:1" },
			{ EPropertyType::Integer, "Number of octaves", "min:0" },
			{ EPropertyType::Boolean, "Rotation invariant", "" },
			{ EPropertyType::Boolean, "Scale invariant", "" },
			{ EPropertyType::Double, "Pattern scale", "min:0.0" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Threshold,
		NumOctaves,
		RotationInvariant,
		ScaleInvariant,
		PatternScale
	};

	int _thresh;
	int _nOctaves;
	bool _rotationInvariant;
	bool _scaleInvariant;
	float _patternScale;

	// Must be pointer since BriskDescriptorExtractor doesn't implement copy/move operator
	// and we want to cache _brisk object
	std::unique_ptr<cv::BriskDescriptorExtractor> _brisk;
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
	typedef DefaultNodeFactory<BriskFeatureDetectorNodeType> BriskFeatureDetectorFactory;
	typedef DefaultNodeFactory<BriskDescriptorExtractorNodeType> BriskDescriptorExtractorFactory;
	typedef DefaultNodeFactory<BriskNodeType> BriskFactory;

	nodeSystem->registerNodeType("Features/Detectors/BRISK", 
		std::unique_ptr<NodeFactory>(new BriskFeatureDetectorFactory()));
	nodeSystem->registerNodeType("Features/Descriptors/BRISK", 
		std::unique_ptr<NodeFactory>(new BriskDescriptorExtractorFactory()));
	nodeSystem->registerNodeType("Features/BRISK",
		std::unique_ptr<NodeFactory>(new BriskFactory()));
}