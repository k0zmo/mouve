#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

class RetainBestFeaturesNodeType : public NodeType
{
public:
	RetainBestFeaturesNodeType()
		: _n(1000)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		if(propId == 0)
		{
			_n = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		if(propId == 0)
			return _n;
		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const KeyPoints& src = reader.readSocket(0).getKeypoints();
		// Acquire output sockets
		KeyPoints& dst = writer.acquireSocket(0).getKeypoints();

		// Validate inputs
		if(src.kpoints.empty())
			return ExecutionStatus(EStatus::Ok);

		// Prepare output structure
		dst.image = src.image;
		dst.kpoints.clear();
		std::copy(std::begin(src.kpoints), std::end(src.kpoints), 
			std::back_inserter(dst.kpoints));

		// Do stuff
		cv::KeyPointsFilter::retainBest(dst.kpoints, _n);

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

		nodeConfig.description = "Retains the specified number of the best keypoints (according to the response).";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}
private:
	int _n;
};

REGISTER_NODE("Features/Retain best", RetainBestFeaturesNodeType)
