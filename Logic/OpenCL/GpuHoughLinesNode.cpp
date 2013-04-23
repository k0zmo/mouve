#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

class GpuHoughLinesNodeType : public GpuNodeType
{
public:
	GpuHoughLinesNodeType()
		: _threshold(80)
		, _showHoughSpace(false)
		, _rhoResolution(1.0f)
		, _thetaResolution(1.0f)
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
		case ID_RhoResolution:
			_rhoResolution = newValue.toFloat();
			return true;
		case ID_ThetaResolution:
			_thetaResolution = newValue.toFloat();
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
		case ID_RhoResolution: return _rhoResolution;
		case ID_ThetaResolution: return _thetaResolution;
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
		clw::Image2D& deviceAccumImage = writer.acquireSocket(1).getDeviceImage();

		int srcWidth = deviceImage.width();
		int srcHeight = deviceImage.height();

		if(srcWidth == 0 || srcHeight == 0)
			return ExecutionStatus(EStatus::Ok);

		int pointsCount = buildPointList(deviceImage, srcWidth, srcHeight);
		if(pointsCount == 0)
			return ExecutionStatus(EStatus::Ok);

		// W akumulatorze plotowac bedziemy krzywe z rodziny
		// rho(theta) = x0*cos(theta) + y0*sin(theta)
		// Zbior wartosci funkcji okreslic mozna poprzez szukanie 
		// ekstremow powyzszej funkcji przy ustalonym x0 i y0:
		// drho/dtheta = 0 co daje: theta_max = atan2(y0,x0)
		// OpenCV szacuje numRho troche nadwyrost: numRho = 2*(src.cols+src.rows)+1
		double thetaMax = atan2(srcHeight, srcWidth);
		double rhoMax = srcWidth*cos(thetaMax) + srcHeight*sin(thetaMax);
		int numRho = cvRound((2 * int(rhoMax + 0.5) + 1) / _rhoResolution);
		int numAngle = cvRound(180 / _thetaResolution);

		if(numAngle <= 0 || numRho <= 0)
			return ExecutionStatus(EStatus::Error, "Wrong rho or theta resolution");

		constructHoughSpace(numRho, numAngle);

		/// TODO: Albo const albo inna metoda bo ta jest zbyt pesymistyczna i bardzo zmienna
		cl_int maxLines = pointsCount / 2;

		extractLines(deviceLines, maxLines, numRho, numAngle);

		if(_showHoughSpace)
			extractHoughSpace(deviceAccumImage, numAngle, numRho);
		else if(!deviceAccumImage.isNull())
			deviceAccumImage = clw::Image2D();

		return ExecutionStatus(EStatus::Ok);
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
			{ EPropertyType::Double, "Rho resolution", "" },
			{ EPropertyType::Double, "Theta resolution", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = 0;
		nodeConfig.module = "opencl";
	}

private:
	int buildPointList(const clw::Image2D& deviceImage, int width, int height) 
	{
		clw::Kernel kernelBuildPointsList = _gpuComputeModule->acquireKernel(_kidBuildPointsList);

		/// TODO: Mozna dac to na gpuInit i PO wykonaniu kernela jako async
		// Zero global counter
		ensureSizeIsEnough(_deviceCounter, sizeof(cl_uint));
		cl_uint* dataZero = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Write);
		*dataZero = 0;
		_gpuComputeModule->queue().unmap(_deviceCounter, dataZero);

		// Build list of non-zero pixels
		ensureSizeIsEnough(_devicePointsList, sizeof(cl_uint) * width * height);
		int pxPerThread = 1; /// TODO: parametrized it
		kernelBuildPointsList.setLocalWorkSize(clw::Grid(32, 4));
		kernelBuildPointsList.setRoundedGlobalWorkSize(clw::Grid((width + pxPerThread - 1)/pxPerThread, height));
		kernelBuildPointsList.setArg(0, deviceImage);
		kernelBuildPointsList.setArg(1, _devicePointsList);
		kernelBuildPointsList.setArg(2, _deviceCounter);
		 _gpuComputeModule->queue().asyncRunKernel(kernelBuildPointsList);

		// Get number of non-zero pixels
		cl_uint* ptr = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Read);
		cl_uint pointsCount = *ptr;
		_gpuComputeModule->queue().asyncUnmap(_deviceCounter, ptr);

		return (int) pointsCount;
	}

	void constructHoughSpace(int numRho, int numAngle) 
	{
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
			kernelAccumLines.setArg(4, 1.0f / _rhoResolution);
			kernelAccumLines.setArg(5, _thetaResolution);
			_gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
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
			kernelAccumLines.setArg(5, 1.0f / _rhoResolution);
			kernelAccumLines.setArg(6, _thetaResolution);
			_gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
		}
	}

	void extractLines(DeviceArray &deviceLines, int maxLines, int numRho, int numAngle) 
	{
		clw::Kernel kernelGetLines = _gpuComputeModule->acquireKernel(_kidGetLines);

		if(deviceLines.isNull()
			// buffer is too small
			|| deviceLines.size() <  maxLines * sizeof(cl_float2)
			// buffer is too big (four times)
			|| deviceLines.size() >  4 * maxLines * sizeof(cl_float2))
		{
			deviceLines = DeviceArray::create(_gpuComputeModule->context(), clw::Access_WriteOnly,
				clw::Location_Device, 2, maxLines, EDataType::Float);
		}

		/// TODO: Mozna dac to na gpuInit i PO wykonaniu kernela jako async (trzeba uzyc innego bufora)
		cl_uint* dataZero = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Write);
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
		kernelGetLines.setArg(6, _rhoResolution);
		kernelGetLines.setArg(7, _thetaResolution);
		_gpuComputeModule->queue().asyncRunKernel(kernelGetLines);

		// Read results
		cl_uint linesCount;
		cl_uint* linesCountPtr = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounter, clw::MapAccess_Read);
		linesCount = *linesCountPtr;
		_gpuComputeModule->queue().unmap(_deviceCounter, linesCountPtr);

		deviceLines.truncate(linesCount);
	}

	void extractHoughSpace(clw::Image2D& deviceAccumImage, int numAngle, int numRho) 
	{
		ensureSizeIsEnough(deviceAccumImage, numAngle, numRho);

		clw::Kernel kernelAccumToImage = _gpuComputeModule->acquireKernel(_kidAccumToImage);
		kernelAccumToImage.setLocalWorkSize(16, 16);
		kernelAccumToImage.setRoundedGlobalWorkSize(numRho, numAngle);
		kernelAccumToImage.setArg(0, _deviceAccum);
		kernelAccumToImage.setArg(1, numRho);
		kernelAccumToImage.setArg(2, deviceAccumImage);
		_gpuComputeModule->queue().runKernel(kernelAccumToImage);
	}

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
		ID_ShowHoughSpace,
		ID_RhoResolution,
		ID_ThetaResolution,
	};

	int _threshold;
	bool _showHoughSpace;
	float _rhoResolution;
	float _thetaResolution;

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