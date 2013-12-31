#pragma once

#if defined(HAVE_OPENCL)

#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/OpenCL/GpuNode.h"

class GpuKuwaharaFilterNodeType : public GpuNodeType
{
public:
	GpuKuwaharaFilterNodeType()
		: _radius(2)
	{
	}

	bool postInit() override
	{
		_kidKuwahara = _gpuComputeModule->registerKernel("kuwahara", "kuwahara.cl");
		return _kidKuwahara != InvalidKernelID;
	}
	
	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case pid::Radius:
			_radius = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case pid::Radius: return _radius;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
		// outputs
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		// validate inputs
		if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
			return ExecutionStatus(EStatus::Ok);

		int width = deviceSrc.width();
		int height = deviceSrc.height();
		auto format = deviceSrc.format();

		// Prepare destination image and structuring element on a device
		ensureSizeIsEnough(deviceDest, width, height);

		clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(_kidKuwahara);
		kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
		kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
		kernelKuwahara.setArg(0, deviceSrc);
		kernelKuwahara.setArg(1, deviceDest);
		kernelKuwahara.setArg(2, _radius);
		_gpuComputeModule->queue().runKernel(kernelKuwahara);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceImage, "output", "Output", "" },
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
		nodeConfig.module = "opencl";
	}

private:
	void ensureSizeIsEnough(clw::Image2D& image, int width, int height)
	{
		if(image.isNull() || 
			image.width() != width	|| 
			image.height() != height)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
				clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8),
				width, height);
		}
	}

private:
	enum class pid
	{
		Radius,
	};

	int _radius;
	KernelID _kidKuwahara;
};

#endif