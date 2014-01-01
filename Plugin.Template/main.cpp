#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/NodeType.h"

class TemplateNodeType : public NodeType
{
public:
	TemplateNodeType()
		: _property(true)
	{
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Property:
			_property = newValue.toBool();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Property: return _property;
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

		if(_property)
		{
			dst = src;
		}

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Property value: %s", _property ? "true" : "false"));
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
			{ EPropertyType::Boolean, "Property", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Template node type";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class pid
	{
		Property,
	};

	bool _property;
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
	typedef DefaultNodeFactory<TemplateNodeType> TemplateFactory;
	nodeSystem->registerNodeType("Test/Template",
		std::unique_ptr<NodeFactory>(new TemplateFactory()));
}