#include "Logic/NodeType.h"
#include "Logic/NodeSystem.h"

#include "cvu.h"

class KuwaharaFilterNodeType : public NodeType
{
public:
	KuwaharaFilterNodeType()
		: _radius(2)
	{
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius:
			_radius = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius: return _radius;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cvu::KuwaharaFilter(src, dst, _radius);
		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Radius", "min: 1, max:15" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Kuwahara filter";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum class pid
	{
		Radius,
	};

	int _radius;
};

class KuwaharaFilterRgbNodeType : public KuwaharaFilterNodeType
{
public:

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImageRgb();
		// outputs
		cv::Mat& dst = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		cvu::KuwaharaFilter(src, dst, _radius);
		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		KuwaharaFilterNodeType::configuration(nodeConfig);

		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageRgb, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Kuwahara filter";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

void registerKuwaharaFilter(NodeSystem* nodeSystem)
{
	typedef DefaultNodeFactory<KuwaharaFilterNodeType> KuwaharaFilterFactory;
	typedef DefaultNodeFactory<KuwaharaFilterRgbNodeType> KuwaharaFilterRgbFactory;

	nodeSystem->registerNodeType("Filters/Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new KuwaharaFilterFactory()));
	nodeSystem->registerNodeType("Filters/Kuwahara RGB filter",
		std::unique_ptr<NodeFactory>(new KuwaharaFilterRgbFactory()));
}