#if defined(HAVE_OPENCL)

#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/OpenCL/GpuNode.h"

#include "cvu.h"

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
		_kidKuwaharaRgb = _gpuComputeModule->registerKernel("kuwaharaRgb", "kuwahara.cl");
		return _kidKuwahara != InvalidKernelID
			&& _kidKuwaharaRgb != InvalidKernelID;
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
		const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
		// outputs
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		// validate inputs
		if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
			return ExecutionStatus(EStatus::Ok);

		int width = deviceSrc.width();
		int height = deviceSrc.height();
		clw::ImageFormat format = deviceSrc.format();

		// Prepare destination image and structuring element on a device
		ensureSizeIsEnough(deviceDest, width, height, format);

		KernelID kid = format.order == clw::EChannelOrder::R ? _kidKuwahara : _kidKuwaharaRgb;
		clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
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
	void ensureSizeIsEnough(clw::Image2D& image, int width,
		int height, const clw::ImageFormat& format)
	{
		if(image.isNull() || 
			image.width() != width || 
			image.height() != height ||
			image.format() != format)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
				format, width, height);
		}
	}

private:
	enum class pid
	{
		Radius,
	};

	int _radius;
	KernelID _kidKuwahara;
	KernelID _kidKuwaharaRgb;
};

class GpuGeneralizedKuwaharaFilterNodeType : public GpuNodeType
{
public:
	GpuGeneralizedKuwaharaFilterNodeType()
		: _radius(6)
		, _N(8)
		, _smoothing(0.3333f)
		, _recreateKernelImage(true)
	{
	}

	bool postInit() override
	{
		_kidGeneralizedKuwahara = _gpuComputeModule->registerKernel("generalizedKuwahara", "kuwahara.cl");
		_kidGeneralizedKuwaharaRgb = _gpuComputeModule->registerKernel("generalizedKuwaharaRgb", "kuwahara.cl");
		return _kidGeneralizedKuwahara != InvalidKernelID
			&& _kidGeneralizedKuwaharaRgb != InvalidKernelID;
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
			_recreateKernelImage = true;
			return true;
		case pid::Smoothing:
			_smoothing = newValue.toFloat();
			_recreateKernelImage = true;
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
		const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
		// outputs
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		// validate inputs
		if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
			return ExecutionStatus(EStatus::Ok);

		int width = deviceSrc.width();
		int height = deviceSrc.height();
		clw::ImageFormat format = deviceSrc.format();

		// Prepare destination image and structuring element on a device
		ensureSizeIsEnough(deviceDest, width, height, format);

		if(_recreateKernelImage)
		{
			cv::Mat kernel_;
			cvu::getGeneralizedKuwaharaKernel(kernel_, _N, _smoothing);
			_kernelImage = _gpuComputeModule->context().createImage2D(clw::EAccess::ReadOnly,
				clw::EMemoryLocation::Device, clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Float),
				kernel_.cols, kernel_.rows, kernel_.data);
			_recreateKernelImage = false;
		}

		KernelID kid = format.order == clw::EChannelOrder::R ? 
			_kidGeneralizedKuwahara : _kidGeneralizedKuwaharaRgb;
		clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
		kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
		kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
		kernelKuwahara.setArg(0, deviceSrc);
		kernelKuwahara.setArg(1, deviceDest);
		kernelKuwahara.setArg(2, _radius);
		kernelKuwahara.setArg(3, _kernelImage);
		kernelKuwahara.setArg(4, _N);
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
			{ EPropertyType::Integer, "Radius", "min: 1, max:20" },
			{ EPropertyType::Integer, "N", "min: 3, max:8" },
			{ EPropertyType::Double, "Smoothing", "min:0.00, max:1.00" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Generalized Kuwahara filter";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.module = "opencl";
	}

private:
	void ensureSizeIsEnough(clw::Image2D& image, int width,
		int height, const clw::ImageFormat& format)
	{
		if(image.isNull() || 
			image.width() != width || 
			image.height() != height ||
			image.format() != format)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
				format, width, height);
		}
	}

private:
	enum class pid
	{
		Radius,
		N,
		Smoothing
	};

	int _radius;
	int _N;
	float _smoothing;

	KernelID _kidGeneralizedKuwahara;
	KernelID _kidGeneralizedKuwaharaRgb;

	clw::Image2D _kernelImage;
	bool _recreateKernelImage;
};

#endif

void registerGpuKuwaharaFilter(NodeSystem* nodeSystem)
{
#if defined(HAVE_OPENCL)
	typedef DefaultNodeFactory<GpuKuwaharaFilterNodeType> GpuKuwaharaFilterFactory;
	typedef DefaultNodeFactory<GpuGeneralizedKuwaharaFilterNodeType> GpuGeneralizedKuwaharaFilterFactory;

	nodeSystem->registerNodeType("OpenCL/Filters/Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new GpuKuwaharaFilterFactory()));
	nodeSystem->registerNodeType("OpenCL/Filters/Generalized Kuwahara filter", 
		std::unique_ptr<NodeFactory>(new GpuGeneralizedKuwaharaFilterFactory()));
#endif
}
