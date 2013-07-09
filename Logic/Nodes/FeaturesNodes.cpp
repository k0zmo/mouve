#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#if defined(HAVE_TBB)
#  include <tbb/tbb.h>
#endif

class RetainBestFeaturesNodeType : public NodeType
{
public:
	RetainBestFeaturesNodeType()
		: _n(1000)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case 0:
			_n = newValue.toUInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case 0: return _n;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& src = reader.readSocket(0).getKeypoints();
		// outputs
		KeyPoints& dst = writer.acquireSocket(0).getKeypoints();

		// validate inputs
		if(src.kpoints.empty())
			return ExecutionStatus(EStatus::Ok);

		dst.image = src.image;
		dst.kpoints.clear();

		std::copy(std::begin(src.kpoints), std::end(src.kpoints), std::back_inserter(dst.kpoints));

#if defined(HAVE_TBB)
		tbb::parallel_sort(std::begin(dst.kpoints), std::end(dst.kpoints),
			[](const cv::KeyPoint& left, const cv::KeyPoint& right) { 
				return left.response < right.response;
		});
#else
		std::sort(std::begin(dst.kpoints), std::end(dst.kpoints),
			[](const cv::KeyPoint& left, const cv::KeyPoint& right) { 
				return left.response < right.response;
		});
#endif

		if(dst.kpoints.size() > size_t(_n))
			dst.kpoints.resize(_n);

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints retained: %d", (int) dst.kpoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "inKp", "Keypoints", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Keypoints, "outKp", "Best", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Best keypoints to retain", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}
private:
	int _n;
};

REGISTER_NODE("Features/Retain best features", RetainBestFeaturesNodeType)
