#pragma once

#if defined(HAVE_OPENCL)

#include "../Prerequisites.h"
#include "../NodeType.h"
#include "GpuNodeModule.h"

class GpuNodeType : public NodeType
{
public:
	GpuNodeType()
		: _gpuComputeModule(nullptr)
	{
	}

	bool init(const std::shared_ptr<NodeModule>& nodeModule) override
	{
		_gpuComputeModule = std::dynamic_pointer_cast<GpuNodeModule, NodeModule>(nodeModule);
		return _gpuComputeModule != nullptr && postInit();
	}

	virtual bool postInit() { return true; }

protected:
	std::shared_ptr<GpuNodeModule> _gpuComputeModule;
};

#endif