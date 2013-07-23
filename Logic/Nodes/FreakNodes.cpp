#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/features2d/features2d.hpp>

class FreakDescriptorExtractorNodeType : public NodeType
{
public:
	FreakDescriptorExtractorNodeType()
		: _orientationNormalized(true)
		, _scaleNormalized(true)
		, _patternScale(22.0f)
		, _nOctaves(4)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_OrientationNormalized:
			_orientationNormalized = newValue.toBool();
			return true;
		case ID_ScaleNormalized:
			_scaleNormalized = newValue.toBool();
			return true;
		case ID_PatternScale:
			_patternScale = newValue.toFloat();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_OrientationNormalized: return _orientationNormalized;
		case ID_ScaleNormalized: return _scaleNormalized;
		case ID_PatternScale: return _patternScale;
		case ID_NumOctaves: return _nOctaves;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& kp = reader.readSocket(0).getKeypoints();

		// Validate inputs
		if(kp.kpoints.empty() || kp.image.empty())
			return ExecutionStatus(EStatus::Ok);

		// Acquire output sockets
		KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
		outKp = kp;

		// Do stuff
		cv::FREAK freak(_orientationNormalized, _scaleNormalized, _patternScale, _nOctaves);
		freak.compute(kp.image, outKp.kpoints, outDescriptors);

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
			{ EPropertyType::Boolean, "Enable orientation normalization", "" },
			{ EPropertyType::Boolean, "Enable scale normalization", "" },
			{ EPropertyType::Double, "Scaling of the description pattern", "min:1.0" },
			{ EPropertyType::Integer, "Number of octaves covered by detected keypoints", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "FREAK (Fast Retina Keypoint) keypoint descriptor.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_OrientationNormalized,
		ID_ScaleNormalized,
		ID_PatternScale,
		ID_NumOctaves,
	};

	bool _orientationNormalized;
	bool _scaleNormalized;
	float _patternScale;
	int _nOctaves;
};
REGISTER_NODE("Features/Descriptors/FREAK", FreakDescriptorExtractorNodeType)