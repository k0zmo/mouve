#include "GpuNode.h"
#include "NodeFactory.h"

static const int NMIXTURES = 5;

class GpuMixtureOfGaussiansNodeType : public GpuNodeType
{
public:
	GpuMixtureOfGaussiansNodeType()
		: _nframe(0)
		, _history(200)
		, _varianceThreshold(6.25f)
		, _backgroundRatio(0.7f)
		, _initialWeight(0.05f)
		, _initialVariance(500) // (15*15*4)
		, _minVariance(0.4f) // (15*15)
	{
	}

	bool restart() override
	{
		_nframe = 0;

		// TUTAJ inicjalizuj bufory???

		if(!_mixtureDataBuffer.isNull())
		{
			// Wyzerowanie
			void* ptr = _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::MapAccess_Write);
			memset(ptr, 0, _mixtureDataBuffer.size());
			_gpuComputeModule->queue().unmap(_mixtureDataBuffer, ptr);
		}

		/*
			Create mixture parameter buffer
		*/
		if(_mixtureParamsBuffer.isNull())
		{
			// Parametry stale dla kernela
			struct MogParams
			{
				float varThreshold;
				float backgroundRatio;
				float w0; // waga dla nowej mikstury
				float var0; // wariancja dla nowej mikstury
				float minVar; // dolny prog mozliwej wariancji
			};

			MogParams mogParams = {
				_varianceThreshold,
				_backgroundRatio,
				_initialWeight,
				_initialVariance,
				_minVariance
			};

			_mixtureParamsBuffer = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadOnly, clw::Location_Device, 
				sizeof(MogParams), &mogParams);
		}

		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImage();
		clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

		int srcWidth = deviceImage.width();
		int srcHeight = deviceImage.height();

		if(srcWidth == 0 || srcHeight == 0)
			return ExecutionStatus(EStatus::Ok);

		clw::Kernel kernelGaussMix = _gpuComputeModule->acquireKernel("mog.cl", "mog_image_unorm");
		if(kernelGaussMix.isNull())
			return ExecutionStatus(EStatus::Error);

		/*
			Create mixture data buffer
		*/
		resetMixturesState(srcWidth * srcHeight);

		if(deviceDest.isNull()
			|| deviceDest.width() != srcWidth
			|| deviceDest.height() != srcHeight)
		{
			// Obraz (w zasadzie maska) pierwszego planu
			deviceDest = _gpuComputeModule->context().createImage2D(
				clw::Access_WriteOnly, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8),
				srcWidth, srcHeight);
		}

		float learningRate = -1.0;

		// Calculate dynamic learning rate (if necessary)
		++_nframe;
		float alpha = learningRate >= 0 && _nframe > 1 
			? learningRate
			: 1.0f/std::min(_nframe, _history);

		kernelGaussMix.setLocalWorkSize(16, 16);
		kernelGaussMix.setRoundedGlobalWorkSize(srcWidth, srcHeight);
		kernelGaussMix.setArg(0, deviceImage);
		kernelGaussMix.setArg(1, deviceDest);
		kernelGaussMix.setArg(2, _mixtureDataBuffer);
		kernelGaussMix.setArg(3, _mixtureParamsBuffer);
		kernelGaussMix.setArg(4, alpha);
		clw::Event evt = _gpuComputeModule->queue().asyncRunKernel(kernelGaussMix);
		_gpuComputeModule->queue().finish();

		double elapsed = (evt.finishTime() - evt.startTime()) * 1e-6;
		return ExecutionStatus(EStatus::Ok, elapsed);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "input", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceImage, "output", "Output", "" },
			//{ ENodeFlowDataType::DeviceImage, "background", "Background", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.flags = Node_HasState | Node_OverridesTimeComputation;
		nodeConfig.module = "opencl";
	}

private:
	void resetMixturesState(int pixNumbers)
	{
		// Dane mikstur (stan wewnetrzny estymatora tla)
		const int mixtureDataSize = NMIXTURES * pixNumbers * 3 * sizeof(float);

		if(_mixtureDataBuffer.isNull()
			|| _mixtureDataBuffer.size() != mixtureDataSize)
		{
			_mixtureDataBuffer = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, mixtureDataSize);

			// Wyzerowanie
			void* ptr = _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::MapAccess_Write);
			memset(ptr, 0, mixtureDataSize);
			_gpuComputeModule->queue().unmap(_mixtureDataBuffer, ptr);
		}
	}

private:
	clw::Buffer _mixtureDataBuffer;
	clw::Buffer _mixtureParamsBuffer;

	int _nframe;
	int _history;
	float _varianceThreshold;
	float _backgroundRatio;
	float _initialWeight;
	float _initialVariance;
	float _minVariance;
};

REGISTER_NODE("OpenCL/Video/Mixture of Gaussians", GpuMixtureOfGaussiansNodeType)