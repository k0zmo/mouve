#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

class GpuMorphologyNodeType : public GpuNodeType
{
public:
	GpuMorphologyNodeType()
		: _op(Erode)
	{
	}

	bool postInit() override
	{
		_kidErode = _gpuComputeModule->registerKernel(
			"morphOp_image_unorm_local_unroll2", "morphOp.cl", "-DERODE_OP");
		_kidDilate = _gpuComputeModule->registerKernel(
			"morphOp_image_unorm_local_unroll2", "morphOp.cl", "-DDILATE_OP");
		return _kidErode != InvalidKernelID
			&& _kidDilate != InvalidKernelID;
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			if(newValue.toInt() < int(EMorphologyOperation::Gradient))
			{
				_op = EMorphologyOperation(newValue.toInt());
				return true;
			}
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Operation: return int(_op);
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
		const cv::Mat& se = reader.readSocket(1).getImage();
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		if(se.cols == 0 || se.rows == 0 || deviceSrc.width() == 0 || deviceSrc.height() == 0)
			return ExecutionStatus(EStatus::Ok);

		// Podfunkcje ktore zwracac beda ExecutionStatus
		// ExecutionStatus dilate(), erode(), 

		int width = deviceSrc.width();
		int height = deviceSrc.height();

		// Prepare destination image and structuring element on a device
		ensureSizeIsEnough(deviceDest, width, height);
		auto selemCoords = structuringElementCoordinates(se);
		int selemCoordsSize = (int) selemCoords.size();
		uploadStructuringElement(selemCoords);	

		// Prepare if necessary buffer for temporary result
		if(_op > int(EMorphologyOperation::Dilate))
			ensureSizeIsEnough(_tmpImage, width, height);

		// Calculate metrics
		int kradx = (se.cols - 1) >> 1;
		int krady = (se.rows - 1) >> 1;		
		clw::Grid grid(deviceSrc.width(), deviceSrc.height());

		// Acquire kernel(s)
		clw::Kernel kernelErode, kernelDilate, kernelSubtract;
		
		if(_op != EMorphologyOperation::Dilate)
			kernelErode = _gpuComputeModule->acquireKernel(_kidErode);
		if(_op > int(EMorphologyOperation::Erode))
			kernelDilate = _gpuComputeModule->acquireKernel(_kidDilate);
		//if(_op > int(EMorphologyOperation::Close))
		//	kernelSubtract = _gpuComputeModule->acquireKernel(_kidSubtract);

		clw::Event evt;

		switch(_op)
		{
		case EMorphologyOperation::Erode:
			evt = runMorphologyKernel(kernelErode, grid, deviceSrc, deviceDest, selemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Dilate:
			evt = runMorphologyKernel(kernelDilate, grid, deviceSrc, deviceDest, selemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Open:
			evt = runMorphologyKernel(kernelErode, grid, deviceSrc, _tmpImage, selemCoordsSize, kradx, krady);
			evt = runMorphologyKernel(kernelDilate, grid, _tmpImage, deviceDest, selemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Close:
			evt = runMorphologyKernel(kernelDilate, grid, deviceSrc, _tmpImage, selemCoordsSize, kradx, krady);
			evt = runMorphologyKernel(kernelErode, grid, _tmpImage, deviceDest, selemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Gradient:
		case EMorphologyOperation::TopHat:
		case EMorphologyOperation::BlackHat:
			return ExecutionStatus(EStatus::Error, "Gradient, TopHat and BlackHat not yet implemented for GPU module");
		}

		// Execute it 
		_gpuComputeModule->queue().finish();

		/// TODO: A bit cheating considering we don't calculate uploading structuring element
		///       In the end we should only upload if that changed from previous node invocation
		double timeElapsed = double(evt.finishTime() - evt.startTime()) * 1e-6;

		return ExecutionStatus(EStatus::Ok, timeElapsed);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "source", "Source", "" },
			{ ENodeFlowDataType::Image, "source", "Structuring element", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceImage, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
				"item: Erode, item: Dilate, item: Open, item: Close,"
				"item: Gradient, item: Top Hat, item: Black Hat"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs a morphology operation on a given image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_OverridesTimeComputation;
		nodeConfig.module = "opencl";
	}

private:
	std::vector<cl_int2> structuringElementCoordinates(const cv::Mat& selem)
	{
		std::vector<cl_int2> coords;
		int seRadiusX = (selem.cols - 1) / 2;
		int seRadiusY = (selem.rows - 1) / 2;
		for(int y = 0; y < selem.rows; ++y)
		{
			const uchar* krow = selem.ptr<uchar>(y);;
			for(int x = 0; x < selem.cols; ++x)
			{
				if(krow[x] == 0)
					continue;
				cl_int2 c = {{x, y}};
				c.s[0] -= seRadiusX;
				c.s[1] -= seRadiusY;
				coords.push_back(c);
			}
		}
		return coords;
	}

	void uploadStructuringElement(const std::vector<cl_int2>& selemCoords)
	{
		/// TODO: Check max costant memory buffer size first
		size_t bmuSize = sizeof(cl_int2) * selemCoords.size();
		if(_deviceStructuringElement.isNull() ||
			_deviceStructuringElement.size() != bmuSize)
		{
			_deviceStructuringElement = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadOnly, clw::Location_Device, bmuSize);
		}

		/// TODO: Benchmark Pinned memory and mapping/direct access
		cl_int2* ptr = static_cast<cl_int2*>(_gpuComputeModule->queue().mapBuffer(
			_deviceStructuringElement, clw::MapAccess_Write));
		const cl_int2* data = selemCoords.data();
		memcpy(ptr, data, bmuSize);
		_gpuComputeModule->queue().unmap(_deviceStructuringElement, ptr);	
		//gpu->queue().writeBuffer(_deviceStructuringElement, selemCoords.data(), 0, bmuSize);	
	}

	void ensureSizeIsEnough(clw::Image2D& image, int width, int height)
	{
		if(image.isNull() 
			|| image.width() != width
			|| image.height() != height)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8),
				width, height);
		}
	}

	clw::Event runMorphologyKernel(clw::Kernel& kernel, const clw::Grid& grid,
		const clw::Image2D& deviceSrc, clw::Image2D& deviceDst, int selemCoordsSize,
		int kradx, int krady)
	{
		// Prepare it for execution
		kernel.setLocalWorkSize(clw::Grid(16, 16));
		kernel.setRoundedGlobalWorkSize(grid);
		kernel.setArg(0, deviceSrc);
		kernel.setArg(1, deviceDst);
		kernel.setArg(2, _deviceStructuringElement);
		kernel.setArg(3, selemCoordsSize);

		int sharedWidth = 16 + 2 * kradx;
		int sharedHeight = 16 + 2 * krady;
		kernel.setArg(4, kradx);
		kernel.setArg(5, krady);
		kernel.setArg(6, clw::LocalMemorySize(sharedWidth * sharedHeight));
		kernel.setArg(7, sharedWidth);
		kernel.setArg(8, sharedHeight);

		// Enqueue the kernel for execution
		return _gpuComputeModule->queue().asyncRunKernel(kernel);
	}

private:
	clw::Buffer _deviceStructuringElement;
	clw::Image2D _tmpImage;
	KernelID _kidErode;
	KernelID _kidDilate;
	///KernelID _kidSubtract;

	enum EMorphologyOperation
	{
		Erode,
		Dilate,
		Open,
		Close,
		Gradient,
		TopHat,
		BlackHat
	};

	enum EPropertyID
	{
		ID_Operation
	};

	EMorphologyOperation _op;
};

REGISTER_NODE("OpenCL/Image/Morphology op.", GpuMorphologyNodeType)

#endif
