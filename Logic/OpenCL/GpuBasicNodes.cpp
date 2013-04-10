#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

/// TODO: Add properties for setting pinned/pageable memory and direct/mapped access
class GpuUploadImageNodeType : public GpuNodeType
{
public:
	GpuUploadImageNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& hostImage = reader.readSocketImage(0);
		clw::Image2D& deviceImage = writer.acquireSocket(0).getDeviceImage();

		/// TODO: For now we assume it's always 1 channel image
		if(hostImage.channels() > 1)
			return ExecutionStatus(EStatus::Error, "Invalid number of channels (should be 1)");

		if(deviceImage.isNull() 
			|| deviceImage.width() != hostImage.cols
			|| deviceImage.height() != hostImage.rows)
		{
			deviceImage = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadOnly, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8),
				hostImage.cols, hostImage.rows);
		}

		/// TODO: Try different methods for data transfers
		bool res = _gpuComputeModule->queue().writeImage2D(deviceImage, hostImage.data, 
			0, 0, hostImage.cols, hostImage.rows, hostImage.step);

		return ExecutionStatus(res ? EStatus::Ok : EStatus::Error);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Host image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceImage, "output", "Device image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Uploads given image from host to device (GPU) memory";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.module = "opencl";
	}
};


/// TODO: Add properties for setting pinned/pageable memory and direct/mapped access
class GpuDownloadImageNodeType : public GpuNodeType
{
public:
	GpuDownloadImageNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImage();
		cv::Mat& hostImage = writer.acquireSocket(0).getImage();

		if(deviceImage.isNull() || deviceImage.size() == 0)
			return ExecutionStatus(EStatus::Ok);

		if(deviceImage.format().order != clw::Order_R)
			return ExecutionStatus(EStatus::Error, "Wrong data type (should be one channel device image)");

		/// TODO: For now we assume it's always 1 channel image
		hostImage.create(deviceImage.height(), deviceImage.width(), CV_8UC1);

		/// TODO: Try different methods for data transfers
		bool res = _gpuComputeModule->queue().readImage2D(deviceImage, hostImage.data, 
			0, 0, hostImage.cols, hostImage.rows, hostImage.step);

		return ExecutionStatus(res ? EStatus::Ok : EStatus::Error);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "input", "Device image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Host image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Download given image device (GPU) to host memory";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.module = "opencl";
	}
};


class GpuUploadArrayNodeType : public GpuNodeType
{
public:
	GpuUploadArrayNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& hostArray = reader.readSocket(0).getArray();
		DeviceArray& deviceArray = writer.acquireSocket(0).getDeviceArray();

		if(hostArray.empty())
			return ExecutionStatus(EStatus::Ok);

		deviceArray.create(_gpuComputeModule->context(), clw::Access_ReadWrite,
			clw::Location_Device, hostArray.cols, hostArray.rows,
			DeviceArray::matToDeviceType(hostArray.type()));
		clw::Event evt = deviceArray.upload(_gpuComputeModule->queue(), hostArray.data);
		evt.waitForFinished();

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Array, "input", "Host array", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceArray, "output", "Device array", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Uploads given array from host to device (GPU) memory";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.module = "opencl";
	}
};


class GpuDownloadArrayNodeType : public GpuNodeType
{
public:
	GpuDownloadArrayNodeType()
	{
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const DeviceArray& deviceImage = reader.readSocket(0).getDeviceArray();
		cv::Mat& hostArray = writer.acquireSocket(0).getArray();

		if(deviceImage.isNull() || deviceImage.size() == 0)
			return ExecutionStatus(EStatus::Ok);

		clw::Event evt = deviceImage.download(_gpuComputeModule->queue(), hostArray);
		evt.waitForFinished();

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceArray, "input", "Device array", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Array, "output", "Host array", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Download given array device (GPU) to host memory";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.module = "opencl";
	}
};

REGISTER_NODE("OpenCL/Download array", GpuDownloadArrayNodeType)
REGISTER_NODE("OpenCL/Upload array", GpuUploadArrayNodeType)
REGISTER_NODE("OpenCL/Download image", GpuDownloadImageNodeType)
REGISTER_NODE("OpenCL/Upload image", GpuUploadImageNodeType)

#endif