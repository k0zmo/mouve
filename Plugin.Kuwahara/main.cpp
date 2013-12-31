#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"

#include "KuwaharaFilterNodeType.h"
#include "GpuKuwaharaFilterNodeType.h"

extern "C" K_DECL_EXPORT int logicVersion()
{
	return 1;
}

extern "C" K_DECL_EXPORT int pluginVersion()
{
	return 1;
}

extern "C" K_DECL_EXPORT void registerPlugin(NodeSystem* nodeSystem)
{
	typedef DefaultNodeFactory<KuwaharaFilterNodeType> KuwaharaFilterFactory;
	typedef DefaultNodeFactory<KuwaharaFilterRgbNodeType> KuwaharaFilterRgbFactory;

	nodeSystem->registerNodeType("Filters/Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new KuwaharaFilterFactory()));
	nodeSystem->registerNodeType("Filters/Kuwahara RGB filter",
		std::unique_ptr<NodeFactory>(new KuwaharaFilterRgbFactory()));

#if defined(HAVE_OPENCL)
	typedef DefaultNodeFactory<GpuKuwaharaFilterNodeType> GpuKuwaharaFilterFactory;
	nodeSystem->registerNodeType("OpenCL/Filters/Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new GpuKuwaharaFilterFactory()));
#endif
}