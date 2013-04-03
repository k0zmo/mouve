#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
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
		return _gpuComputeModule != nullptr;
	}

protected:
	std::shared_ptr<GpuNodeModule> _gpuComputeModule;
};