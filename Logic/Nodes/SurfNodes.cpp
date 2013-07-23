#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/nonfree/features2d.hpp>

class SurfFeatureDetectorNodeType : public NodeType
{
public:
	SurfFeatureDetectorNodeType()
		: _hessianThreshold(400.0)
		, _nOctaves(4)
		, _nOctaveLayers(2)
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
		cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves, _nOctaveLayers);
		detector.detect(src, kp.kpoints);
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
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "min:1" },
			{ EPropertyType::Integer, "Number of octave layers", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Extracts Speeded Up Robust Features from an image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumOctaveLayers
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nOctaveLayers;
};

class SurfDescriptorExtractorNodeType : public NodeType
{
public:
	SurfDescriptorExtractorNodeType()
		: _extended(false)
		, _upright(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
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
		case ID_Extended: return _extended;
		case ID_Upright: return _upright;
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
		cv::SurfDescriptorExtractor extractor;
		extractor.compute(kp.image, outKp.kpoints, outDescriptors);

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
			{ EPropertyType::Boolean, "Extended", "" },
			{ EPropertyType::Boolean, "Upright", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Describes local features using SURF algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Extended,
		ID_Upright
	};

	bool _extended;
	bool _upright;
};

class SurfNodeType : public NodeType
{
public:
	SurfNodeType()
		: _hessianThreshold(400.0)
		, _nOctaves(4)
		, _nOctaveLayers(2)
		, _extended(false)
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
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::SURF surf(_hessianThreshold, _nOctaves,
			_nOctaveLayers, _extended, _upright);
		surf(src, cv::noArray(), kp.kpoints, descriptors);
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
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
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

		nodeConfig.description = "Extracts Speeded Up Robust Features and computes their descriptors from an image.";
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

REGISTER_NODE("Features/Descriptors/SURF", SurfDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/SURF", SurfFeatureDetectorNodeType)
REGISTER_NODE("Features/SURF", SurfNodeType)
