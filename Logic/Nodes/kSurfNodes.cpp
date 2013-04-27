#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include "ksurf.h"

class kSurfNodeType : public NodeType
{
public:
	kSurfNodeType()
		: _hessianThreshold(40.0)
		, _nOctaves(4)
		, _nScales(4)
		, _initSampling(1)
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
		case ID_NumScales:
			_nScales = newValue.toInt();
			return true;
		case ID_InitSampling:
			_initSampling = newValue.toInt();
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
		case ID_NumScales: return _nScales;
		case ID_InitSampling: return _initSampling;
		case ID_Upright : return _upright;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::KeyPoints& keypoints = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		kSURF surf(_hessianThreshold, _nOctaves, _nScales, _initSampling, _upright);
		surf(src, cv::noArray(), keypoints, descriptors);

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
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "min:1" },
			{ EPropertyType::Integer, "Number of scales", "min:1" },
			{ EPropertyType::Integer, "Initial sampling rate", "min:1" },
			{ EPropertyType::Boolean, "Upright", "" },
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
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumScales,
		ID_InitSampling,
		ID_Upright
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nScales;
	int _initSampling;
	bool _upright;
};

REGISTER_NODE("Features/kSURF", kSurfNodeType)
