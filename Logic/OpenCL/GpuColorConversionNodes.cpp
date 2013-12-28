#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

#include "Logic/Nodes/CV.h"

class GpuBayerToGrayNodeType : public GpuNodeType
{
public:
	GpuBayerToGrayNodeType()
		: _redGain(1.0)
		, _greenGain(1.0)
		, _blueGain(1.0)
		, _BayerCode(cvu::EBayerCode::RG)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case ID_BayerFormat:
			_BayerCode = newValue.toEnum().cast<cvu::EBayerCode>();
			return true;
		case ID_RedGain:
			_redGain = newValue.toDouble();
			return true;
		case ID_GreenGain:
			_greenGain = newValue.toDouble();
			return true;
		case ID_BlueGain:
			_blueGain = newValue.toDouble();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_BayerFormat: return _BayerCode;
		case ID_RedGain: return _redGain;
		case ID_GreenGain: return _greenGain;
		case ID_BlueGain: return _blueGain;
		}

		return NodeProperty();
	}

	bool postInit() override
	{
		// On AMD up from AMD APP 2.7 which supports 1.2 OpenCL we can use templates and other c++ goodies
		string opts;
		if(_gpuComputeModule->device().platform().vendorEnum() == clw::Vendor_AMD
			&& _gpuComputeModule->device().platform().version() >= clw::Version_1_2)
		{
			opts = "-DTEMPLATES_SUPPORTED -x clc++";
		}

		// Local version is a bit faster
		_kidConvertRG2Gray = _gpuComputeModule->registerKernel("convert_rg2gray_local", "bayer.cl", opts);
		_kidConvertGB2Gray = _gpuComputeModule->registerKernel("convert_gb2gray_local", "bayer.cl", opts);
		_kidConvertGR2Gray = _gpuComputeModule->registerKernel("convert_gr2gray_local", "bayer.cl", opts);
		_kidConvertBG2Gray = _gpuComputeModule->registerKernel("convert_bg2gray_local", "bayer.cl", opts);

		return _kidConvertRG2Gray != InvalidKernelID
			&& _kidConvertGB2Gray != InvalidKernelID
			&& _kidConvertGR2Gray != InvalidKernelID
			&& _kidConvertBG2Gray != InvalidKernelID;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& input = reader.readSocket(0).getDeviceImage();
		clw::Image2D& output = writer.acquireSocket(0).getDeviceImage();

		int imageWidth = input.width();
		int imageHeight = input.height();

		if(imageWidth == 0 || imageHeight == 0)
			return ExecutionStatus(EStatus::Ok);

		clw::Kernel kernelConvertBayer2Gray;
		switch(_BayerCode)
		{
		case cvu::EBayerCode::RG:
			kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertRG2Gray);
			break;
		case cvu::EBayerCode::GB:
			kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertGB2Gray);
			break;
		case cvu::EBayerCode::GR:
			kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertGR2Gray);
			break;
		case cvu::EBayerCode::BG:
			kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertBG2Gray);
			break;
		}

		if(kernelConvertBayer2Gray.isNull())
			return ExecutionStatus(EStatus::Error, "Bad bayer code");

		// Ensure output image size is enough
		if(output.isNull() || output.width() != imageWidth || output.height() != imageHeight)
		{
			output = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device, 
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8), 
				imageWidth, imageHeight);
		}

		cl_float3 gains = { (float) _redGain, (float) _greenGain, (float) _blueGain };
		int sharedWidth = 16 + 2;
		int sharedHeight = 16 + 2;

		kernelConvertBayer2Gray.setLocalWorkSize(16, 16);
		kernelConvertBayer2Gray.setRoundedGlobalWorkSize(imageWidth, imageHeight);
		kernelConvertBayer2Gray.setArg(0, input);
		kernelConvertBayer2Gray.setArg(1, output);
		kernelConvertBayer2Gray.setArg(2, gains);
		kernelConvertBayer2Gray.setArg(3, clw::LocalMemorySize(sharedWidth*sharedHeight*sizeof(float)));
		kernelConvertBayer2Gray.setArg(4, sharedWidth);
		kernelConvertBayer2Gray.setArg(5, sharedHeight);

		_gpuComputeModule->queue().asyncRunKernel(kernelConvertBayer2Gray);
		_gpuComputeModule->queue().finish();

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceImage, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Bayer format", "item: BG, item: GB, item: RG, item: GR" },
			{ EPropertyType::Double, "Red gain", "min:0.0, max:4.0, step:0.01" },
			{ EPropertyType::Double, "Green gain", "min:0.0, max:4.0, step:0.01" },
			{ EPropertyType::Double, "Blue gain", "min:0.0, max:4.0, step:0.01" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs demosaicing from Bayer pattern image to RGB image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.module = "opencl";
	}

private:
	enum EPropertyID
	{
		ID_BayerFormat,
		ID_RedGain,
		ID_GreenGain,
		ID_BlueGain,
	};

	double _redGain;
	double _greenGain;
	double _blueGain;
	cvu::EBayerCode _BayerCode;

private:
	KernelID _kidConvertRG2Gray;
	KernelID _kidConvertGB2Gray;
	KernelID _kidConvertGR2Gray;
	KernelID _kidConvertBG2Gray;
};

REGISTER_NODE("OpenCL/Format conversion/Gray de-bayer", GpuBayerToGrayNodeType)

#endif