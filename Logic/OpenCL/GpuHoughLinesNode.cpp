#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

class GpuHoughLinesNodeType : public GpuNodeType
{
public:
	GpuHoughLinesNodeType()
		: _threshold(80)
		, _showHoughSpace(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue)
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toUInt();
			return true;
		case ID_ShowHoughSpace:
			_showHoughSpace = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_ShowHoughSpace: return _showHoughSpace;
		}

		return QVariant();
	}

	bool postInit() override
	{
		_kidBuildPointsList = _gpuComputeModule->registerKernel("buildPointsList", "hough.cl");
		_kidAccumLines = _gpuComputeModule->registerKernel("accumLines", "hough.cl");
		_kidAccumLinesShared = _gpuComputeModule->registerKernel("accumLines_shared", "hough.cl");
		_kidGetLines = _gpuComputeModule->registerKernel("getLines", "hough.cl");
		_kidAccumToImage = _gpuComputeModule->registerKernel("accumToImage", "hough.cl");

		return _kidBuildPointsList != InvalidKernelID
			&& _kidAccumLines != InvalidKernelID
			&& _kidAccumLinesShared != InvalidKernelID
			&& _kidGetLines != InvalidKernelID
			&& _kidAccumToImage != InvalidKernelID;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImage();
		DeviceArray& deviceLines = writer.acquireSocket(0).getDeviceArray();

		int srcWidth = deviceImage.width();
		int srcHeight = deviceImage.height();

		if(srcWidth == 0 || srcHeight == 0)
			return ExecutionStatus(EStatus::Ok);

		/* 
			1. Build list of non-zero pixels
		*/
		clw::Kernel kernelBuildPointsList = _gpuComputeModule->acquireKernel(_kidBuildPointsList);

		/// TODO: Mozna dac to na gpuInit i PO wykonaniu kernela jako async
		// Zero global counter
		ensureSizeIsEnough(_deviceCounter, sizeof(cl_uint));
		cl_uint* dataZero = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Write);
		*dataZero = 0;
		_gpuComputeModule->queue().unmap(_deviceCounter, dataZero);

		ensureSizeIsEnough(_devicePointsList, sizeof(cl_uint) * srcWidth * srcHeight);
		int pxPerThread = 1;
		kernelBuildPointsList.setLocalWorkSize(clw::Grid(32, 4));
		kernelBuildPointsList.setRoundedGlobalWorkSize(clw::Grid((srcWidth + pxPerThread - 1)/pxPerThread, srcHeight));
		kernelBuildPointsList.setArg(0, deviceImage);
		kernelBuildPointsList.setArg(1, _devicePointsList);
		kernelBuildPointsList.setArg(2, _deviceCounter);
		clw::Event evt = _gpuComputeModule->queue().asyncRunKernel(kernelBuildPointsList);

		/*
			2. Get number of non-zero pixels
		*/
		cl_uint* ptr = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Read);
		cl_uint pointsCount = *ptr;
		_gpuComputeModule->queue().unmap(_deviceCounter, ptr);

		if(pointsCount == 0)
			return ExecutionStatus(EStatus::Ok);

		/*
			3. Contruct Hough space
		*/
		// W akumulatorze plotowac bedziemy krzywe z rodziny
		// rho(theta) = x0*cos(theta) + y0*sin(theta)
		// Zbior wartosci funkcji okreslic mozna poprzez szukanie 
		// ekstremow powyzszej funkcji przy ustalonym x0 i y0:
		// drho/dtheta = 0 co daje: theta_max = atan2(y0,x0)
		double thetaMax = atan2(srcHeight, srcWidth);
		double rhoMax = srcWidth*cos(thetaMax) + srcHeight*sin(thetaMax);
		int numRho = 2 * int(rhoMax + 0.5) + 1;
		int numAngle = 180;
		// OpenCV szacuje numRho troche nadwyrost: numRho = 2*(src.cols+src.rows)+1

		ensureSizeIsEnough(_deviceAccum, numAngle * numRho * sizeof(cl_int));

		uint64_t requiredSharedSize = numRho * sizeof(cl_int);

		if(!_gpuComputeModule->isLocalMemorySufficient(requiredSharedSize))
		{
			clw::Kernel kernelAccumLines = _gpuComputeModule->acquireKernel(_kidAccumLines);

			//cl_int pattern = 0;
			//clEnqueueFillBuffer(_gpuComputeModule->queue().commandQueueId(), _deviceAccum.memoryId(), &pattern,
			//	sizeof(pattern), 0, numAngle * numRho * sizeof(cl_int), 0, nullptr, nullptr);
			cl_int* accumPtr = (cl_int*) _gpuComputeModule->queue().mapBuffer(_deviceAccum, clw::MapAccess_Write);
			memset(accumPtr, 0, sizeof(cl_int) * numAngle * numRho);
			_gpuComputeModule->queue().unmap(_deviceAccum, accumPtr);

			kernelAccumLines.setLocalWorkSize(clw::Grid(256));
			kernelAccumLines.setRoundedGlobalWorkSize(clw::Grid(256 * numAngle));
			kernelAccumLines.setArg(0, _devicePointsList);
			kernelAccumLines.setArg(1, _deviceCounter);
			kernelAccumLines.setArg(2, _deviceAccum);
			kernelAccumLines.setArg(3, numRho);
			evt = _gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
		}
		else
		{
			clw::Kernel kernelAccumLines = _gpuComputeModule->acquireKernel(_kidAccumLinesShared);

			kernelAccumLines.setLocalWorkSize(clw::Grid(256));
			kernelAccumLines.setRoundedGlobalWorkSize(clw::Grid(256 * numAngle));
			kernelAccumLines.setArg(0, _devicePointsList);
			kernelAccumLines.setArg(1, _deviceCounter);
			kernelAccumLines.setArg(2, _deviceAccum);
			kernelAccumLines.setArg(3, clw::LocalMemorySize(requiredSharedSize));
			kernelAccumLines.setArg(4, numRho);
			evt = _gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
		}

		/*
			4. Retrieve lines from Hough space accumulator
		*/
		clw::Kernel kernelGetLines = _gpuComputeModule->acquireKernel(_kidGetLines);
		if(kernelGetLines.isNull())
			return ExecutionStatus(EStatus::Error);

		/// TODO: Albo const albo inna metoda bo ta jest zbyt pesymistyczna i bardzo zmienna
		cl_int maxLines = pointsCount / 2;

		if(deviceLines.isNull()
			// buffer is too small
			|| deviceLines.size() <  maxLines * sizeof(cl_float2)
			// buffer is too big (four times)
			|| deviceLines.size() >  4 * maxLines * sizeof(cl_float2))
			//|| deviceLines.size() !=  maxLines * sizeof(cl_float2))
		{
			deviceLines = DeviceArray::create(_gpuComputeModule->context(), clw::Access_WriteOnly,
				clw::Location_Device, 2, maxLines, EDataType::Float);
		}

		/// TODO: Mozna dac to na gpuInit i PO wykonaniu kernela jako async (trzeba uzyc innego bufora)
		dataZero = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Write);
		*dataZero = 0;
		_gpuComputeModule->queue().unmap(_deviceCounter, dataZero);

		kernelGetLines.setLocalWorkSize(clw::Grid(32, 8));
		kernelGetLines.setRoundedGlobalWorkSize(clw::Grid(numRho, numAngle));
		kernelGetLines.setArg(0, _deviceAccum);
		kernelGetLines.setArg(1, deviceLines.buffer());
		kernelGetLines.setArg(2, _deviceCounter);
		kernelGetLines.setArg(3, _threshold);
		kernelGetLines.setArg(4, maxLines);
		kernelGetLines.setArg(5, numRho);
		evt = _gpuComputeModule->queue().asyncRunKernel(kernelGetLines);

		/*
			5. Read results
		*/
		cl_uint linesCount;
		//_gpuComputeModule->queue.asyncReadBuffer(_deviceCounter, &linesCount, 0, sizeof(cl_uint));
		cl_uint* linesCountPtr = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Read);
		linesCount = *linesCountPtr;
		_gpuComputeModule->queue().unmap(_deviceCounter, linesCountPtr);

		deviceLines.truncate(linesCount);

#if 0
		if(linesCount > 0)
		{
			std::vector<cl_float2> lines(linesCount);

			//_gpuComputeModule->queue().readBuffer(_deviceLines,
			//	lines.data(), 0, linesCount * sizeof(cl_float2));

			cl_float2* linesPtr = (cl_float2*) _gpuComputeModule->queue().mapBuffer(_deviceLines, clw::MapAccess_Read);
			memcpy(lines.data(), linesPtr, linesCount * sizeof(cl_float2));
			_gpuComputeModule->queue().unmap(_deviceLines, linesPtr);
		}
#endif

		// Finish enqueued jobs
		//_gpuComputeModule->queue().finish();
		double elapsed = double(evt.finishTime() - evt.startTime()) * 1e-6;

		
		//if(1)
		//{
		//	cv::Mat& accum = writer.acquireSocket(1).getImage();
		//	accum.create(numAngle, numRho, CV_32SC1);

		//	_gpuComputeModule->queue().readBuffer(_deviceAccum, accum.data, 0,
		//		numAngle * numRho * sizeof(cl_int));

		//	double maxVal;
		//	cv::minMaxIdx(accum, nullptr, &maxVal);
		//	accum = accum.t();
		//	accum.convertTo(accum, CV_8UC1);
		//}

		if(_showHoughSpace)
		{
			clw::Image2D& deviceAccumImage = writer.acquireSocket(1).getDeviceImage();
			ensureSizeIsEnough(deviceAccumImage, numAngle, numRho);

			clw::Kernel kernelAccumToImage = _gpuComputeModule->acquireKernel(_kidAccumToImage);
			kernelAccumToImage.setLocalWorkSize(16, 16);
			kernelAccumToImage.setRoundedGlobalWorkSize(numRho, numAngle);
			kernelAccumToImage.setArg(0, _deviceAccum);
			kernelAccumToImage.setArg(1, numRho);
			kernelAccumToImage.setArg(2, deviceAccumImage);
			_gpuComputeModule->queue().runKernel(kernelAccumToImage);
		}
		

		return ExecutionStatus(EStatus::Ok, elapsed);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "input", "Binary image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::DeviceArray, "lines", "Lines", "" },
			{ ENodeFlowDataType::DeviceImage, "houghSpace", "Hough space", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Threshold", "min:1" },
			{ EPropertyType::Boolean, "Show Hough space", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = 0;//Node_OverridesTimeComputation;
		nodeConfig.module = "opencl";
	}

private:
	void ensureSizeIsEnough(clw::Buffer& buffer, size_t size)
	{
		/// TODO: Or buffer.size() < size
		if(buffer.isNull() || buffer.size() != size)
		{
			buffer = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, size);
		}
	}

	void ensureSizeIsEnough(clw::Image2D& image, int width, int height)
	{
		if(image.isNull()
			|| image.width() != width
			|| image.height() != height)
		{
			image = _gpuComputeModule->context().createImage2D(
				clw::Access_WriteOnly, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Normalized_UInt8),
				width, height);
		}
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_ShowHoughSpace
	};

	int _threshold;
	bool _showHoughSpace;

	/// TODO:
	///float _rhoResolution;
	///float _thetaResolution;

private:
	clw::Buffer _devicePointsList;
	clw::Buffer _deviceCounter;
	clw::Buffer _deviceAccum;

	KernelID _kidBuildPointsList;
	KernelID _kidAccumLines;
	KernelID _kidAccumLinesShared;
	KernelID _kidGetLines;
	KernelID _kidAccumToImage;
};

REGISTER_NODE("OpenCL/Features/Hough Lines", GpuHoughLinesNodeType)

#endif