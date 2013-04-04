#include "GpuNode.h"
#include "NodeFactory.h"

class GpuMorphologyNodeType : public GpuNodeType
{
public:
	GpuMorphologyNodeType()
	{
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

		// Acquire kernel
		clw::Kernel kernel = _gpuComputeModule->acquireKernel("morphOp.cl", "morphOp_image_unorm_local_unroll2");
		if(kernel.isNull())
			return ExecutionStatus(EStatus::Error);

		prepareDestinationImage(deviceDest, deviceSrc.width(), deviceSrc.height());
		auto selemCoords = structuringElementCoordinates(se);
		uploadStructuringElement(selemCoords);	

		// Prepare it for execution
		kernel.setLocalWorkSize(clw::Grid(16, 16));
		kernel.setRoundedGlobalWorkSize(clw::Grid(deviceSrc.width(), deviceSrc.height()));
		kernel.setArg(0, deviceSrc);
		kernel.setArg(1, deviceDest);
		kernel.setArg(2, _deviceStructuringElement);
		kernel.setArg(3, (int) selemCoords.size());

		int kradx = (se.cols - 1) >> 1;
		int krady = (se.rows - 1) >> 1;
		int sharedWidth = 16 + 2 * kradx;
		int sharedHeight = 16 + 2 * krady;
		kernel.setArg(4, kradx);
		kernel.setArg(5, krady);
		kernel.setArg(6, clw::LocalMemorySize(sharedWidth * sharedHeight));
		kernel.setArg(7, sharedWidth);
		kernel.setArg(8, sharedHeight);

		// Execute the kernel
		clw::Event evt = _gpuComputeModule->queue().asyncRunKernel(kernel);
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

	void prepareDestinationImage(clw::Image2D& deviceDest, int width, int height)
	{
		if(deviceDest.isNull() 
			|| deviceDest.width() != width
			|| deviceDest.height() != height)
		{
			deviceDest = _gpuComputeModule->context().createImage2D(
				clw::Access_WriteOnly, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8),
				width, height);
		}
	}

private:
	clw::Buffer _deviceStructuringElement;
};

REGISTER_NODE("OpenCL/Image/Morphology op.", GpuMorphologyNodeType)