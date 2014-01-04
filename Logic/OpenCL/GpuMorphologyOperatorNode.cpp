#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "Logic/NodeFactory.h"
#include "Kommon/Hash.h"

class GpuMorphologyOperatorNodeType : public GpuNodeType
{
public:
	GpuMorphologyOperatorNodeType()
		: _op(EMorphologyOperation::Erode)
		, _sElemHash(0)
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

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Operation:
			if(newValue.toEnum().data() < Enum(EMorphologyOperation::Gradient).data())
			{
				_op = newValue.toEnum().cast<EMorphologyOperation>();
				return true;
			}
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::Operation: return _op;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
		const cv::Mat& sElem = reader.readSocket(1).getImage();
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		if(sElem.cols == 0 || sElem.rows == 0 || deviceSrc.width() == 0 || deviceSrc.height() == 0)
			return ExecutionStatus(EStatus::Ok);

		// Podfunkcje ktore zwracac beda ExecutionStatus
		// ExecutionStatus dilate(), erode(), 

		int width = deviceSrc.width();
		int height = deviceSrc.height();

		// Prepare destination image and structuring element on a device
		ensureSizeIsEnough(deviceDest, width, height);

		int sElemCoordsSize = prepareStructuringElement(sElem);
		if(!sElemCoordsSize)
			return ExecutionStatus(EStatus::Error, "Structuring element is too big to fit in constant memory");

		// Prepare if necessary buffer for temporary result
		if(_op > EMorphologyOperation::Dilate)
			ensureSizeIsEnough(_tmpImage, width, height);

		// Acquire kernel(s)
		clw::Kernel kernelErode, kernelDilate, kernelSubtract;
		
		if(_op != EMorphologyOperation::Dilate)
			kernelErode = _gpuComputeModule->acquireKernel(_kidErode);
		if(_op > EMorphologyOperation::Erode)
			kernelDilate = _gpuComputeModule->acquireKernel(_kidDilate);
		//if(_op > EMorphologyOperation::Close)
		//	kernelSubtract = _gpuComputeModule->acquireKernel(_kidSubtract);

		clw::Event evt;

		// Calculate metrics
		int kradx = (sElem.cols - 1) >> 1;
		int krady = (sElem.rows - 1) >> 1;		
		clw::Grid grid(deviceSrc.width(), deviceSrc.height());

		switch(_op)
		{
		case EMorphologyOperation::Erode:
			evt = runMorphologyKernel(kernelErode, grid, deviceSrc, deviceDest, sElemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Dilate:
			evt = runMorphologyKernel(kernelDilate, grid, deviceSrc, deviceDest, sElemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Open:
			evt = runMorphologyKernel(kernelErode, grid, deviceSrc, _tmpImage, sElemCoordsSize, kradx, krady);
			evt = runMorphologyKernel(kernelDilate, grid, _tmpImage, deviceDest, sElemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Close:
			evt = runMorphologyKernel(kernelDilate, grid, deviceSrc, _tmpImage, sElemCoordsSize, kradx, krady);
			evt = runMorphologyKernel(kernelErode, grid, _tmpImage, deviceDest, sElemCoordsSize, kradx, krady);
			break;
		case EMorphologyOperation::Gradient:
		case EMorphologyOperation::TopHat:
		case EMorphologyOperation::BlackHat:
			return ExecutionStatus(EStatus::Error, "Gradient, TopHat and BlackHat not yet implemented for GPU module");
		}

		// Execute it 
		_gpuComputeModule->queue().finish();

		return ExecutionStatus(EStatus::Ok);
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
		nodeConfig.module = "opencl";
	}

private:
	int prepareStructuringElement(const cv::Mat& sElem) 
	{
		vector<cl_int2> sElemCoords = structuringElementCoordinates(sElem);
		size_t bmuSize = sizeof(cl_int2) * sElemCoords.size();
		if(!_gpuComputeModule->isConstantMemorySufficient(bmuSize))
			return 0;

		// Check if this is the same structuring element by hashing
		if(!_deviceStructuringElement.isNull() && _deviceStructuringElement.size() == bmuSize)
		{
			uint32_t hash = calculateStructuringElemCoordsHash(sElemCoords);

			if(hash != _sElemHash)
			{
				uploadStructuringElement(sElemCoords);	
				_sElemHash = calculateStructuringElemCoordsHash(sElemCoords);
			}
		}
		else
		{
			uploadStructuringElement(sElemCoords);	
			_sElemHash = calculateStructuringElemCoordsHash(sElemCoords);
		}

		return (int) sElemCoords.size();
	}

	vector<cl_int2> structuringElementCoordinates(const cv::Mat& sElem)
	{
		vector<cl_int2> coords;
		int seRadiusX = (sElem.cols - 1) / 2;
		int seRadiusY = (sElem.rows - 1) / 2;
		for(int y = 0; y < sElem.rows; ++y)
		{
			const uchar* krow = sElem.ptr<uchar>(y);;
			for(int x = 0; x < sElem.cols; ++x)
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

	uint32_t calculateStructuringElemCoordsHash(const vector<cl_int2>& sElemCoords) 
	{
		return SuperFastHash(reinterpret_cast<const char*>(sElemCoords.data()), 
			static_cast<int>(sElemCoords.size() * sizeof(cl_int2)));
	}

	void uploadStructuringElement(const vector<cl_int2>& sElemCoords)
	{
		size_t bmuSize = sizeof(cl_int2) * sElemCoords.size();
		if(_deviceStructuringElement.isNull() ||
			_deviceStructuringElement.size() != bmuSize)
		{
			_deviceStructuringElement = _gpuComputeModule->context().createBuffer(
				clw::EAccess::ReadOnly, clw::EMemoryLocation::Device, bmuSize);

			_pinnedStructuringElement = _gpuComputeModule->context().createBuffer(
				clw::EAccess::ReadWrite, clw::EMemoryLocation::AllocHostMemory, bmuSize);
		}

		cl_int2* ptr = static_cast<cl_int2*>(_gpuComputeModule->queue().mapBuffer(
			_pinnedStructuringElement, clw::EMapAccess::Write));
		memcpy(ptr, sElemCoords.data(), bmuSize);
		_gpuComputeModule->queue().asyncUnmap(_pinnedStructuringElement, ptr);
		_gpuComputeModule->queue().asyncCopyBuffer(_pinnedStructuringElement, _deviceStructuringElement);
	}

	void ensureSizeIsEnough(clw::Image2D& image, int width, int height)
	{
		if(image.isNull() 
			|| image.width() != width
			|| image.height() != height)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
				clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8),
				width, height);
		}
	}

	clw::Event runMorphologyKernel(clw::Kernel& kernel, const clw::Grid& grid,
		const clw::Image2D& deviceSrc, clw::Image2D& deviceDst, int sElemCoordsSize,
		int kradx, int krady)
	{
		// Prepare it for execution
		kernel.setLocalWorkSize(clw::Grid(16, 16));
		kernel.setRoundedGlobalWorkSize(grid);
		kernel.setArg(0, deviceSrc);
		kernel.setArg(1, deviceDst);
		kernel.setArg(2, _deviceStructuringElement);
		kernel.setArg(3, sElemCoordsSize);

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
	clw::Buffer _pinnedStructuringElement;
	clw::Image2D _tmpImage;
	KernelID _kidErode;
	KernelID _kidDilate;
	///KernelID _kidSubtract;

	enum class EMorphologyOperation
	{
		Erode,
		Dilate,
		Open,
		Close,
		Gradient,
		TopHat,
		BlackHat
	};

	enum class pid
	{
		Operation
	};

	EMorphologyOperation _op;
	uint32_t _sElemHash;
};

REGISTER_NODE("OpenCL/Morphology/Operator", GpuMorphologyOperatorNodeType)

#endif
