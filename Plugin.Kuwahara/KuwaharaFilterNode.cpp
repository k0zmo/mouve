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

class GeneralizedKuwaharaFilterNodeType : public NodeType
{
public:
	GeneralizedKuwaharaFilterNodeType()
		: _radius(6)
		, _N(8)
		, _smoothing(0.3333f)
	{
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius:
			_radius = newValue.toInt();
			return true;
		case pid::N:
			_N = newValue.toInt();
			return true;
		case pid::Smoothing:
			_smoothing = newValue.toFloat();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius: return _radius;
		case pid::N: return _N;
		case pid::Smoothing: return _smoothing;
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

		cvu::generalizedKuwaharaFilter(src, dst, _radius, _N, _smoothing);
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
			{ EPropertyType::Integer, "Radius", "min: 1, max:20" },
			{ EPropertyType::Integer, "N", "min: 3, max:8" },
			{ EPropertyType::Double, "Smoothing", "min:0.00, max:1.00" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Generalized Kuwahara filter";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum class pid
	{
		Radius,
		N,
		Smoothing
	};

	int _radius;
	int _N;
	float _smoothing;
};

class AnisotropicKuwaharaFilterNodeType : public NodeType
{
public:
	AnisotropicKuwaharaFilterNodeType()
		: _radius(6)
		, _N(8)
		, _smoothing(0.3333f)
	{
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius:
			_radius = newValue.toInt();
			return true;
		case pid::N:
			_N = newValue.toInt();
			return true;
		case pid::Smoothing:
			_smoothing = newValue.toFloat();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Radius: return _radius;
		case pid::N: return _N;
		case pid::Smoothing: return _smoothing;
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

		cvu::anisotropicKuwaharaFilter(src, dst, _radius, _N, _smoothing);
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
			{ EPropertyType::Integer, "Radius", "min: 1, max:20" },
			{ EPropertyType::Integer, "N", "min: 3, max:8" },
			{ EPropertyType::Double, "Smoothing", "min:0.00, max:1.00" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Anisotropic Kuwahara filter";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	enum class pid
	{
		Radius,
		N,
		Smoothing
	};

	int _radius;
	int _N;
	float _smoothing;
};

void registerKuwaharaFilter(NodeSystem* nodeSystem)
{
	typedef DefaultNodeFactory<KuwaharaFilterNodeType> KuwaharaFilterFactory;
	typedef DefaultNodeFactory<GeneralizedKuwaharaFilterNodeType> GeneralizedKuwaharaFilterFactory;
	typedef DefaultNodeFactory<AnisotropicKuwaharaFilterNodeType> AnisotropicKuwaharaFilterFactory;

	nodeSystem->registerNodeType("Filters/Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new KuwaharaFilterFactory()));
	nodeSystem->registerNodeType("Filters/Generalized Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new GeneralizedKuwaharaFilterFactory()));
	nodeSystem->registerNodeType("Filters/Anisotropic Kuwahara filter",
		std::unique_ptr<NodeFactory>(new AnisotropicKuwaharaFilterFactory()));
}