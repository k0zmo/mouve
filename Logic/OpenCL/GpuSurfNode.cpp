#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

#include <sstream>
#include <opencv2/imgproc/imgproc.hpp>

#include "Logic/Nodes/ksurf.h"

using namespace std;

// Use exactly the same filters sizes as H. Bay in his paper
#define FILTER_SIZE_BAY 1

//static const int KEYPOINTS_MAX = 16384;
static const int KEYPOINTS_MAX = 65535;
// Size of a 1st box filter in 1st octave (9x9 is default)
static const int FILTER_SIZE_BASE = 9;
static const int FILTER_SIZE_BASE_INCREASE = 6;

struct ScaleSpaceLayer_cl
{
	// Width of the scale space layer
	int width;
	// Height of the scale space layer
	int height;
	// Sample step - equals to 1 or multiply of 2
	int sampleStep;
	// Size of box filter this layer was blurred with
	int filterSize;
	// determinant of Hesse's matrix
	clw::Buffer hessian_cl; 
	// trace (Lxx + Lyy) of Hesse's matrix - which is Laplacian
	clw::Buffer laplacian_cl;
};

string warpSizeDefine(const std::shared_ptr<GpuNodeModule>& gpuModule)
{
	ostringstream strm;
	strm << "-DWARP_SIZE=";
	if(gpuModule->device().deviceType() != clw::Gpu)
	{
		strm << 1;
	}
	else
	{
		clw::EPlatformVendor vendor = gpuModule->device().platform().vendorEnum();

		if(vendor == clw::Vendor_AMD)
			strm << gpuModule->device().wavefrontWidth();
		// How to get this for Intel HD Graphics?
		else if(vendor == clw::Vendor_Intel)
			strm << 1;
		else if(vendor == clw::Vendor_NVIDIA)
			strm << gpuModule->device().warpSize();
	}

	return strm.str();
}

class GpuSurfNodeType : public GpuNodeType
{
public:
	GpuSurfNodeType()
		: _hessianThreshold(35.0)
		, _nOctaves(4)
		, _nScales(4)
		, _initSampling(1)
		, _msurf(true)
		, _upright(false)
		, _downloadDescriptors(true)
		, _nTotalLayers(0)
		, _constantsUploaded(false)
	{
	}

	bool postInit() override
	{
		string opts = warpSizeDefine(_gpuComputeModule);
		_kidFillImage = _gpuComputeModule->registerKernel("fill_image_uint", "fill.cl");
		_kidMultiScan_horiz = _gpuComputeModule->registerKernel("multiscan_horiz_image", "integral.cl", opts);
		_kidMultiScan_vert = _gpuComputeModule->registerKernel("multiscan_vert_image", "integral.cl", opts);

		ostringstream strm;
		strm << " -DKEYPOINT_MAX=" << KEYPOINTS_MAX;
		opts += strm.str();

		_kidBuildScaleSpace = _gpuComputeModule->registerKernel("buildScaleSpace", "surf.cl", opts);
		_kidFindScaleSpaceMaxima = _gpuComputeModule->registerKernel("findScaleSpaceMaxima", "surf.cl", opts);
		_kidUprightKeypointOrientation = _gpuComputeModule->registerKernel("uprightKeypointOrientation", "surf.cl", opts);
		_kidFindKeypointOrientation = _gpuComputeModule->registerKernel("findKeypointOrientation", "surf.cl", opts);
		_kidCalculateDescriptors = _gpuComputeModule->registerKernel("calculateDescriptors", "surf.cl", opts);
		_kidCalculateDescriptorsMSurf = _gpuComputeModule->registerKernel("calculateDescriptorsMSURF", "surf.cl", opts);
		_kidNormalizeDescriptors = _gpuComputeModule->registerKernel("normalizeDescriptors", "surf.cl", opts);
			
		return _kidFillImage != InvalidKernelID &&
			_kidMultiScan_horiz != InvalidKernelID &&
			_kidMultiScan_vert != InvalidKernelID &&
			_kidBuildScaleSpace != InvalidKernelID &&
			_kidFindScaleSpaceMaxima != InvalidKernelID &&
			_kidUprightKeypointOrientation != InvalidKernelID &&
			_kidFindKeypointOrientation != InvalidKernelID &&
			_kidCalculateDescriptors != InvalidKernelID &&
			_kidCalculateDescriptorsMSurf != InvalidKernelID &&
			_kidNormalizeDescriptors != InvalidKernelID;
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_HessianThreshold:
			_hessianThreshold = newValue.toDouble();
			return true;
		case ID_NumOctaves:
			_nOctaves = newValue.toInt();
			return true;
		case ID_NumScales:
			_nScales = newValue.toInt();
			return true;
		case ID_InitSampling:
			_initSampling = newValue.toInt();
			return true;
		case ID_MSurfDescriptor:
			_msurf = newValue.toBool();
			return true;
		case ID_Upright:
			_upright = newValue.toBool();
			return true;
		case ID_DownloadDescriptors:
			_downloadDescriptors = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_HessianThreshold: return _hessianThreshold;
		case ID_NumOctaves: return _nOctaves;
		case ID_NumScales: return _nScales;
		case ID_InitSampling: return _initSampling;
		case ID_MSurfDescriptor: return _msurf;
		case ID_Upright: return _upright;
		case ID_DownloadDescriptors: return _downloadDescriptors;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImage();
		KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
		cv::Mat& descriptors = writer.acquireSocket(1).getArray();
		DeviceArray& descriptors_dev = writer.acquireSocket(2).getDeviceArray();

		int imageWidth = deviceImage.width();
		int imageHeight = deviceImage.height();
		if(imageWidth == 0 || imageHeight == 0)
			return ExecutionStatus(EStatus::Ok);

		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "SURF");

		if(!_constantsUploaded)
			uploadSurfConstants();

		ensureKeypointsBufferIsEnough();

		// Zero keypoints counter (use pinned memory)
		// Another way would be to map pinned buffer (allocated on a host) 
		// and read the device buffer using just obtained pointer. If data transfer is single-direction
		// we don't need to map and unmap every time - just after creation and before destruction of a buffer

		// From AMD APP OpenCL Programming Guide:
		// pinnedBuffer = clCreateBuffer(CL_MEM_ALLOC_HOST_PTR or CL_MEM_USE_HOST_PTR)
		// deviceBuffer = clCreateBuffer()

		// 1)
		// void* pinnedMemory = clEnqueueMapBuffer(pinnedBuffer) -> can be done only once
		// clEnqueueRead/WriteBuffer(deviceBuffer, pinnedMemory)
		// clEnqueueUnmapMemObject(pinnedBuffer, pinnedMemory) -> can be done only once
		//
		// 2) 
		// void* pinnedMemory = clEnqueueMapBuffer(pinnedBuffer)
		// [Application writes or modifies memory (host memory bandwith)]
		// clEnqueueUnmapMemObject(pinnedBuffer, pinnedMemory)
		// clEnqueueCopyBuffer(pinnedBuffer, deviceBuffer)
		//  - or -
		// clEnqueueCopyBuffer(deviceBuffer, pinnedBuffer)
		// void* pinnedMemory = clEnqueueMapBuffer(pinnedBuffer)
		// [Application reads memory (host memory bandwith)]
		// clEnqueueUnmapMemObject(pinnedBuffer, pinnedMemory)


		// On AMD these are the same solutions
		int* intPtr = (int*) _gpuComputeModule->queue().mapBuffer(_pinnedKeypointsCount_cl, clw::MapAccess_Write);
		if(!intPtr)
			return ExecutionStatus(EStatus::Error, "Couldn't mapped keypoints counter buffer to host address space");
		intPtr[0] = 0;
		_gpuComputeModule->queue().asyncUnmap(_pinnedKeypointsCount_cl, intPtr);
		_gpuComputeModule->queue().asyncCopyBuffer(_pinnedKeypointsCount_cl, _keypointsCount_cl);

		convertImageToIntegral(deviceImage, imageWidth, imageHeight);
		prepareScaleSpaceLayers(imageWidth, imageHeight);
		buildScaleSpace(imageWidth, imageHeight);
		findScaleSpaceMaxima();

		kp.image.create(imageHeight, imageWidth, CV_8UC1);
		_gpuComputeModule->dataQueue().asyncReadImage2D(deviceImage, kp.image.data, (int) kp.image.step);
		_gpuComputeModule->dataQueue().flush();

		_gpuComputeModule->beginPerfMarker("Read number of keypoints", "SURF");

		// Read keypoints counter (use pinned memory)
		_gpuComputeModule->queue().asyncCopyBuffer(_keypointsCount_cl, _pinnedKeypointsCount_cl);
		intPtr = (int*) _gpuComputeModule->queue().mapBuffer(_pinnedKeypointsCount_cl, clw::MapAccess_Read);
		if(!intPtr)
			return ExecutionStatus(EStatus::Error, "Couldn't mapped keypoints counter buffer to host address space");
		int keypointsCount = min(intPtr[0], KEYPOINTS_MAX);
		_gpuComputeModule->queue().asyncUnmap(_pinnedKeypointsCount_cl, intPtr);

		_gpuComputeModule->endPerfMarker();

		if(keypointsCount > 0)
		{
			if(!_upright)
				findKeypointOrientation(keypointsCount);
			else
				uprightKeypointOrientation(keypointsCount);
			calculateDescriptors(keypointsCount);

			GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Read results", "SURF");

			// Start copying descriptors to pinned buffer
			if(_downloadDescriptors)
				_gpuComputeModule->queue().asyncCopyBuffer(_descriptors_cl, _pinnedDescriptors_cl);

			vector<KeyPoint> kps = downloadKeypoints(keypointsCount);
			kp.kpoints = transformKeyPoint(kps);

			descriptors_dev = DeviceArray::createFromBuffer(_descriptors_cl, 64, keypointsCount, EDataType::Float);

			if(_downloadDescriptors)
			{
				descriptors.create(keypointsCount, 64, CV_32F);
				float* floatPtr = (float*) _gpuComputeModule->queue().mapBuffer(_pinnedDescriptors_cl, clw::MapAccess_Read);
				if(!floatPtr)
					return ExecutionStatus(EStatus::Error, "Couldn't mapped descriptors buffer to host address space");
				if(descriptors.step == 64*sizeof(float))
				{
					//copy(floatPtr, floatPtr + 64*keypointsCount, descriptors.ptr<float>());
					memcpy(descriptors.ptr<float>(), floatPtr, sizeof(float)*64 * keypointsCount);
				}
				else
				{
					for(int row = 0; row < keypointsCount; ++row)
						//copy(floatPtr + 64*row, floatPtr + 64*row + 64, descriptors.ptr<float>(row));
						memcpy(descriptors.ptr<float>(row), floatPtr + 64*row, sizeof(float)*64);
				}

				_gpuComputeModule->queue().asyncUnmap(_pinnedDescriptors_cl, floatPtr);
			}

			// Finish downloading input image
			_gpuComputeModule->dataQueue().finish();
		}
		else
		{
			kp = KeyPoints();
			descriptors = cv::Mat();
			descriptors_dev = DeviceArray();
		}

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Keypoints detected: %d", (int) kp.kpoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::DeviceImage, "image", "Image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			/// TODO: Just for now
			{ ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
			{ ENodeFlowDataType::Array, "output", "Descriptors", "" },
			{ ENodeFlowDataType::DeviceArray, "output", "Device descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Hessian threshold", "" },
			{ EPropertyType::Integer, "Number of octaves", "min:1" },
			{ EPropertyType::Integer, "Number of scales", "min:1" },
			{ EPropertyType::Integer, "Initial sampling rate", "min:1" },
			{ EPropertyType::Boolean, "MSURF descriptor", "" },
			{ EPropertyType::Boolean, "Upright", "" },
			{ EPropertyType::Boolean, "Download descriptors", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.module = "opencl";
		nodeConfig.flags = 0;
	}

protected:
	void uploadSurfConstants()
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Upload constants", "SURF");

		cv::Mat gauss_ = cv::getGaussianKernel(13, 2.5, CV_32F);
		const int nOriSamples = 109;

		_gaussianWeights_cl = _gpuComputeModule->context().createBuffer(
			clw::Access_ReadOnly, clw::Location_Device, sizeof(float)*nOriSamples);
		_samplesCoords_cl = _gpuComputeModule->context().createBuffer(
			clw::Access_ReadOnly, clw::Location_Device, sizeof(cl_int2)*nOriSamples);

		float* gaussianPtr = (float*) _gpuComputeModule->queue().mapBuffer(_gaussianWeights_cl, clw::MapAccess_Write);
		cl_int2* samplesPtr = (cl_int2*) _gpuComputeModule->queue().mapBuffer(_samplesCoords_cl, clw::MapAccess_Write);
		if(!gaussianPtr || !samplesPtr)
			return;
		int index = 0;

		// Builds list of pixel coordinates to process and their Gaussian-based weight
		for(int y = -6; y <= 6; ++y)
		{
			for(int x = -6; x <= 6; ++x)
			{
				if(x*x + y*y < 36)
				{
					cl_int2 coords = {x,y};
					samplesPtr[index] = coords;
					gaussianPtr[index] = gauss_.at<float>(y+6) * gauss_.at<float>(x+6);
					++index;
				}
			}
		}
		CV_Assert(nOriSamples == index);

		_constantsUploaded = true;

		_gpuComputeModule->queue().asyncUnmap(_gaussianWeights_cl, gaussianPtr);
		_gpuComputeModule->queue().asyncUnmap(_samplesCoords_cl, samplesPtr);
	}

	bool prepareScaleSpaceLayers(int imageWidth, int imageHeight)
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Prepare layers", "SURF");

		const int nTotalLayers = _nOctaves * _nScales;
		if(!nTotalLayers)
			return false;

		if(nTotalLayers != _nTotalLayers)
		{
			_scaleLayers.clear();
			_scaleLayers.resize(nTotalLayers);
		}

		int scaleBaseFilterSize = FILTER_SIZE_BASE;

		// Initialize scale space layers with their properties
		for(int octave = 0; octave < _nOctaves; ++octave)
		{
			for(int scale = 0; scale < _nScales; ++scale)
			{
				auto&& layer = _scaleLayers[octave*_nScales + scale];

				layer.width = imageWidth >> octave;
				layer.height = imageHeight >> octave;
				layer.sampleStep = _initSampling << octave;
#if FILTER_SIZE_BAY == 1
				layer.filterSize = scaleBaseFilterSize + (scale * FILTER_SIZE_BASE_INCREASE << octave);
#else
				layer.filterSize = (FILTER_SIZE_BASE + FILTER_SIZE_BASE_INCREASE*scale) << octave;
#endif
				// Allocate required memory for scale layers
				ensureHessianAndLaplacianBufferIsEnough(layer);
			}

			scaleBaseFilterSize += FILTER_SIZE_BASE_INCREASE << octave;
		}

		_nTotalLayers = nTotalLayers;
		return true;
	}

	void ensureKeypointsBufferIsEnough()
	{
		if(_keypoints_cl.isNull() || _keypoints_cl.size() != KEYPOINTS_MAX*sizeof(KeyPoint))
		{
			_keypoints_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, KEYPOINTS_MAX*sizeof(KeyPoint));
			_pinnedKeypoints_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_AllocHostMemory, KEYPOINTS_MAX*sizeof(KeyPoint));
		}

		if(_keypointsCount_cl.isNull())
		{
			_keypointsCount_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, sizeof(int));
			_pinnedKeypointsCount_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_AllocHostMemory, sizeof(int));
		}
	}

	void ensureHessianAndLaplacianBufferIsEnough(ScaleSpaceLayer_cl& layer)
	{
		const size_t layerSizeBytes = layer.width * layer.height * sizeof(float);

		if(layer.hessian_cl.isNull() || layer.hessian_cl.size() != layerSizeBytes)
		{
			layer.hessian_cl = _gpuComputeModule->context().createBuffer(clw::Access_ReadWrite, 
				clw::Location_Device, layerSizeBytes);
			layer.laplacian_cl = _gpuComputeModule->context().createBuffer(clw::Access_ReadWrite, 
				clw::Location_Device, layerSizeBytes);
		}
	}

	virtual void convertImageToIntegral(const clw::Image2D& srcImage_cl, int imageWidth, int imageHeight)
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Integral image", "SURF");

		// Create if necessary image on a device
		if(_tempImage_cl.isNull() || 
			_tempImage_cl.width() != imageWidth ||
			_tempImage_cl.height() != imageHeight)
		{
			_tempImage_cl = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Unnormalized_UInt32),
				imageWidth, imageHeight);

			_imageIntegral_cl = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Unnormalized_UInt32),
				imageWidth+1, imageHeight+1);

			clw::Kernel kernelFillImage = _gpuComputeModule->acquireKernel(_kidFillImage);

			kernelFillImage.setLocalWorkSize(16,16);
			kernelFillImage.setRoundedGlobalWorkSize(imageWidth+1,imageHeight+1);
			kernelFillImage.setArg(0, _imageIntegral_cl);
			kernelFillImage.setArg(1, 0);
			_gpuComputeModule->queue().asyncRunKernel(kernelFillImage);
		}

		if(_gpuComputeModule->device().deviceType() != clw::Cpu)
		{
			clw::Kernel kernelMultiScan_horiz = _gpuComputeModule->acquireKernel(_kidMultiScan_horiz);
			clw::Kernel kernelMultiScan_vert = _gpuComputeModule->acquireKernel(_kidMultiScan_vert);

			kernelMultiScan_horiz.setLocalWorkSize(256,1);
			kernelMultiScan_horiz.setRoundedGlobalWorkSize(256,imageHeight);
			kernelMultiScan_horiz.setArg(0, srcImage_cl);
			kernelMultiScan_horiz.setArg(1, _tempImage_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelMultiScan_horiz);

			kernelMultiScan_vert.setLocalWorkSize(256,1);
			kernelMultiScan_vert.setRoundedGlobalWorkSize(256,imageWidth);
			kernelMultiScan_vert.setArg(0, _tempImage_cl);
			kernelMultiScan_vert.setArg(1, _imageIntegral_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelMultiScan_vert);
		}
		else
		{
			cv::Mat srcHost(imageHeight, imageWidth, CV_8UC1), integralHost;
			_gpuComputeModule->queue().readImage2D(srcImage_cl, srcHost.data, srcHost.step);
			cv::integral(srcHost, integralHost);
			_gpuComputeModule->queue().writeImage2D(_imageIntegral_cl, integralHost.data, integralHost.step);
		}
	}

	virtual void buildScaleSpace(int imageWidth, int imageHeight)
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Build scale space", "SURF");

		clw::Kernel kernelBuildScaleSpace = _gpuComputeModule->acquireKernel(_kidBuildScaleSpace);		

		for(int octave = 0; octave < _nOctaves; ++octave)
		{
			for(int scale = 0; scale < _nScales; ++scale)
			{
				auto&& layer = _scaleLayers[octave*_nScales + scale];

				if(imageWidth < layer.filterSize
				|| imageHeight < layer.filterSize)
					continue;

				// Promien filtru: dla 9x9 jest to 4
				int fw = (layer.filterSize - 1) / 2;
				// wielkosc 'garbu' (1/3 wielkosci filtru)
				int lobe = layer.filterSize / 3;
				// Promien poziomy (dla Lxx) garbu
				int lw = (lobe - 1) / 2;
				// Promien pionowy (dla Lxx) garbu
				int lh = lobe - 1;
				// Przesuniecie od 0,0 do centra garbu dla Lxy
				int xyOffset = lw + 1;

				// Odwrotnosc pola filtru - do normalizacji
				float invNorm = 1.0f / (layer.filterSize*layer.filterSize);

				// Wielkosc ramki dla ktorej nie bedzie liczone det(H) oraz tr(H)
				int layerMargin = layer.filterSize / (2 * layer.sampleStep);

				// Moze zdarzyc sie tak (od drugiej oktawy jest tak zawsze) ze przez sub-sampling
				// zaczniemy filtrowac od zbyt malego marginesu - ta wartosc (zwykle 1) pozwala tego uniknac
				int stepOffset = fw - (layerMargin * layer.sampleStep);

				// Ilosc probek w poziomie i pionie dla danej oktawy i skali
				int samples_x = 1 + (imageWidth - layer.filterSize) / layer.sampleStep;
				int samples_y = 1 + (imageHeight - layer.filterSize) / layer.sampleStep;

				kernelBuildScaleSpace.setLocalWorkSize(16, 16);
				kernelBuildScaleSpace.setGlobalWorkOffset(layerMargin, layerMargin);
				kernelBuildScaleSpace.setRoundedGlobalWorkSize(samples_x, samples_y);
				kernelBuildScaleSpace.setArg( 0, _imageIntegral_cl);
				kernelBuildScaleSpace.setArg( 1, samples_x);
				kernelBuildScaleSpace.setArg( 2, samples_y);
				kernelBuildScaleSpace.setArg( 3, layer.hessian_cl);
				kernelBuildScaleSpace.setArg( 4, layer.laplacian_cl);
				kernelBuildScaleSpace.setArg( 5, layer.width);
				kernelBuildScaleSpace.setArg( 6, fw);
				kernelBuildScaleSpace.setArg( 7, lw);
				kernelBuildScaleSpace.setArg( 8, lh);
				kernelBuildScaleSpace.setArg( 9, xyOffset);
				kernelBuildScaleSpace.setArg(10, invNorm);
				kernelBuildScaleSpace.setArg(11, layer.sampleStep);
				kernelBuildScaleSpace.setArg(12, stepOffset);

				_gpuComputeModule->queue().asyncRunKernel(kernelBuildScaleSpace);
			}
		}
	}

	void findScaleSpaceMaxima() 
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Nonmaxima supression", "SURF");

		clw::Kernel kernelFindScaleSpaceMaxima = _gpuComputeModule->acquireKernel(_kidFindScaleSpaceMaxima);

		for(int octave = 0; octave < _nOctaves; ++octave)
		{
			for(int scale = 1; scale < _nScales-1; ++scale)
			{
				int indexMiddle = octave*_nScales + scale;
				auto&& layerBottom = _scaleLayers[indexMiddle - 1];
				auto&& layerMiddle = _scaleLayers[indexMiddle];
				auto&& layerTop = _scaleLayers[indexMiddle + 1];

				int layerMargin = (layerTop.filterSize) / (2 * layerTop.sampleStep) + 1;
				int scaleDiff = layerMiddle.filterSize - layerBottom.filterSize;

				int samples_y = layerMiddle.height - 2*layerMargin;
				int samples_x = layerMiddle.width - 2*layerMargin;

				if(samples_x <= 0 || samples_y <= 0)
					continue;

				kernelFindScaleSpaceMaxima.setLocalWorkSize(16, 16);
				kernelFindScaleSpaceMaxima.setGlobalWorkOffset(layerMargin, layerMargin);
				kernelFindScaleSpaceMaxima.setRoundedGlobalWorkSize(samples_x, samples_y);

				kernelFindScaleSpaceMaxima.setArg( 0, samples_x);                 // samples_x
				kernelFindScaleSpaceMaxima.setArg( 1, samples_y);                 // samples_y
				kernelFindScaleSpaceMaxima.setArg( 2, layerBottom.hessian_cl);    // hessian0
				kernelFindScaleSpaceMaxima.setArg( 3, layerMiddle.hessian_cl);    // hessian1
				kernelFindScaleSpaceMaxima.setArg( 4, layerTop.hessian_cl);       // hessian2
				kernelFindScaleSpaceMaxima.setArg( 5, layerMiddle.laplacian_cl);  // laplacian1
				kernelFindScaleSpaceMaxima.setArg( 6, layerMiddle.width);         // pitch
				kernelFindScaleSpaceMaxima.setArg( 7, (float) _hessianThreshold); // hessianThreshold
				kernelFindScaleSpaceMaxima.setArg( 8, layerMiddle.sampleStep);    // sampleStep
				kernelFindScaleSpaceMaxima.setArg( 9, layerMiddle.filterSize);    // filterSize
				kernelFindScaleSpaceMaxima.setArg(10, scaleDiff);                 // scaleDiff
				kernelFindScaleSpaceMaxima.setArg(11, _keypointsCount_cl);        // keypointsCount
				kernelFindScaleSpaceMaxima.setArg(12, _keypoints_cl);             // keypoints

				_gpuComputeModule->queue().asyncRunKernel(kernelFindScaleSpaceMaxima);
			}
		}
	}

	void findKeypointOrientation(int keypointsCount) 
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Orientation", "SURF");

		clw::Kernel kernelFindKeypointOrientation = _gpuComputeModule->acquireKernel(_kidFindKeypointOrientation);

		kernelFindKeypointOrientation.setLocalWorkSize(64);
		kernelFindKeypointOrientation.setGlobalWorkSize(64*keypointsCount);
		kernelFindKeypointOrientation.setArg(0, _imageIntegral_cl);
		kernelFindKeypointOrientation.setArg(1, _keypoints_cl);
		kernelFindKeypointOrientation.setArg(2, _samplesCoords_cl);
		kernelFindKeypointOrientation.setArg(3, _gaussianWeights_cl);
		_gpuComputeModule->queue().asyncRunKernel(kernelFindKeypointOrientation);
	}

	void uprightKeypointOrientation(int keypointsCount)
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Orientation", "SURF");

		clw::Kernel kernelUprightKeypointOrientation = _gpuComputeModule->acquireKernel(_kidUprightKeypointOrientation);

		kernelUprightKeypointOrientation.setLocalWorkSize(64);
		kernelUprightKeypointOrientation.setRoundedGlobalWorkSize(keypointsCount);
		kernelUprightKeypointOrientation.setArg(0, _keypoints_cl);
		kernelUprightKeypointOrientation.setArg(1, keypointsCount);
		_gpuComputeModule->queue().asyncRunKernel(kernelUprightKeypointOrientation);
	}

	void calculateDescriptors(int keypointsCount) 
	{
		GpuPerformanceMarker marker(_gpuComputeModule->performanceMarkersInitialized(), "Descriptors", "SURF");

		if(_descriptors_cl.isNull() || 
			// Allocate only if buffer is too small
			_descriptors_cl.size() < sizeof(float)*64*keypointsCount)
		{
			_descriptors_cl = _gpuComputeModule->context().createBuffer(clw::Access_WriteOnly, 
				clw::Location_Device, sizeof(float)*64*keypointsCount);
			_pinnedDescriptors_cl = _gpuComputeModule->context().createBuffer(clw::Access_WriteOnly, 
				clw::Location_AllocHostMemory, sizeof(float)*64*keypointsCount);
		}

		if(_msurf)
		{
			clw::Kernel kernelCalculateDescriptorsMSurf = _gpuComputeModule->acquireKernel(_kidCalculateDescriptorsMSurf);
			kernelCalculateDescriptorsMSurf.setLocalWorkSize(9, 9);
			kernelCalculateDescriptorsMSurf.setGlobalWorkSize(keypointsCount * 9, 4*4 * 9);
			kernelCalculateDescriptorsMSurf.setArg(0, _imageIntegral_cl);
			kernelCalculateDescriptorsMSurf.setArg(1, _keypoints_cl);
			kernelCalculateDescriptorsMSurf.setArg(2, _descriptors_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelCalculateDescriptorsMSurf);
		}
		else
		{
			clw::Kernel kernelCalculateDescriptors = _gpuComputeModule->acquireKernel(_kidCalculateDescriptors);
			kernelCalculateDescriptors.setLocalWorkSize(5, 5);
			kernelCalculateDescriptors.setGlobalWorkSize(keypointsCount * 5, 4*4 * 5);
			kernelCalculateDescriptors.setArg(0, _imageIntegral_cl);
			kernelCalculateDescriptors.setArg(1, _keypoints_cl);
			kernelCalculateDescriptors.setArg(2, _descriptors_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelCalculateDescriptors);
		}

		clw::Kernel kernelNormalizeDescriptors = _gpuComputeModule->acquireKernel(_kidNormalizeDescriptors);

		kernelNormalizeDescriptors.setLocalWorkSize(64);
		kernelNormalizeDescriptors.setGlobalWorkSize(keypointsCount * 64);
		kernelNormalizeDescriptors.setArg(0, _descriptors_cl);
		_gpuComputeModule->queue().asyncRunKernel(kernelNormalizeDescriptors);
	}

	vector<KeyPoint> downloadKeypoints(int keypointsCount) 
	{
		vector<KeyPoint> kps(keypointsCount);
		_gpuComputeModule->queue().asyncCopyBuffer(_keypoints_cl, _pinnedKeypoints_cl);
		float* ptrBase = (float*) _gpuComputeModule->queue().mapBuffer(_pinnedKeypoints_cl, clw::MapAccess_Read);
		if(!ptrBase)
			return kps;

		float* ptr = ptrBase + KEYPOINTS_MAX * 0;
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].x = *ptr++;

		ptr = ptrBase + KEYPOINTS_MAX * 1;
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].y = *ptr++;

		ptr = ptrBase + KEYPOINTS_MAX * 2;
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].scale = *ptr++;

		ptr = ptrBase + KEYPOINTS_MAX * 3;
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].response = *ptr++;

		int* iptr = (int*) (ptrBase + KEYPOINTS_MAX * 4);
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].laplacian = *iptr++;

		ptr = ptrBase + KEYPOINTS_MAX * 5;
		for(int i = 0; i < keypointsCount; ++i)
			kps[i].orientation = *ptr++;

		_gpuComputeModule->queue().asyncUnmap(_pinnedKeypoints_cl, ptrBase);

		return kps;
	}

protected:
	enum EPropertyID
	{
		ID_HessianThreshold,
		ID_NumOctaves,
		ID_NumScales,
		ID_InitSampling,
		ID_MSurfDescriptor,
		ID_Upright,
		ID_DownloadDescriptors
	};

	double _hessianThreshold;
	int _nOctaves;
	int _nScales;
	int _initSampling;
	bool _msurf;
	bool _upright;
	bool _downloadDescriptors;

	KernelID _kidFillImage;
	KernelID _kidMultiScan_horiz;
	KernelID _kidMultiScan_vert;
	KernelID _kidBuildScaleSpace;
	KernelID _kidFindScaleSpaceMaxima;
	KernelID _kidUprightKeypointOrientation;
	KernelID _kidFindKeypointOrientation;
	KernelID _kidCalculateDescriptors;
	KernelID _kidCalculateDescriptorsMSurf;
	KernelID _kidNormalizeDescriptors;

	clw::Image2D _srcImage_cl;
	clw::Image2D _tempImage_cl;
	clw::Image2D _imageIntegral_cl;

	vector<ScaleSpaceLayer_cl> _scaleLayers;
	int _nTotalLayers;

	clw::Buffer _gaussianWeights_cl;
	clw::Buffer _samplesCoords_cl;
	bool _constantsUploaded;

	clw::Buffer _keypoints_cl;
	clw::Buffer _keypointsCount_cl;
	clw::Buffer _descriptors_cl;

	clw::Buffer _pinnedKeypoints_cl;
	clw::Buffer _pinnedKeypointsCount_cl;
	clw::Buffer _pinnedDescriptors_cl;
};

// Temporary solution for too much VGPR usage on GCN hardware when image2d_t is used
class GpuSurfBufferNodeType : public GpuSurfNodeType
{
public:
	bool postInit() override
	{
		bool res = GpuSurfNodeType::postInit();
		if(res)
		{	
			ostringstream strm;
			if(_gpuComputeModule->device().supportsExtension("cl_ext_atomic_counters_32"))
				strm << " -DUSE_ATOMIC_COUNTERS";
			strm << " -DKEYPOINT_MAX=" << KEYPOINTS_MAX;
			string opts = warpSizeDefine(_gpuComputeModule) + strm.str();

			_kidBuildScaleSpace = _gpuComputeModule->registerKernel("buildScaleSpace_buffer", "surf.cl", opts);
			return _kidBuildScaleSpace != InvalidKernelID;
		}
		return res;
	}

protected:
	void convertImageToIntegral(const clw::Image2D& srcImage_cl, int imageWidth, int imageHeight) override
	{
		// Create if necessary image on a device
		if(_tempImage_cl.isNull() || 
			_tempImage_cl.width() != imageWidth ||
			_tempImage_cl.height() != imageHeight)
		{
			_tempImage_cl = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Unnormalized_UInt32),
				imageWidth, imageHeight);

			_imageIntegral_cl = _gpuComputeModule->context().createImage2D(
				clw::Access_ReadWrite, clw::Location_Device,
				clw::ImageFormat(clw::Order_R, clw::Type_Unnormalized_UInt32),
				imageWidth+1, imageHeight+1);

			_integralBufferPitch = imageWidth+1;
			_imageIntegralBuffer_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadOnly, clw::Location_Device,
				sizeof(cl_uint) * _integralBufferPitch * (imageHeight+1));

			clw::Kernel kernelFillImage = _gpuComputeModule->acquireKernel(_kidFillImage);

			kernelFillImage.setLocalWorkSize(16,16);
			kernelFillImage.setRoundedGlobalWorkSize(imageWidth+1,imageHeight+1);
			kernelFillImage.setArg(0, _imageIntegral_cl);
			kernelFillImage.setArg(1, 0);
			_gpuComputeModule->queue().asyncRunKernel(kernelFillImage);
		}

		if(_gpuComputeModule->device().deviceType() != clw::Cpu)
		{
			clw::Kernel kernelMultiScan_horiz = _gpuComputeModule->acquireKernel(_kidMultiScan_horiz);
			clw::Kernel kernelMultiScan_vert = _gpuComputeModule->acquireKernel(_kidMultiScan_vert);

			kernelMultiScan_horiz.setLocalWorkSize(256,1);
			kernelMultiScan_horiz.setRoundedGlobalWorkSize(256,imageHeight);
			kernelMultiScan_horiz.setArg(0, srcImage_cl);
			kernelMultiScan_horiz.setArg(1, _tempImage_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelMultiScan_horiz);

			kernelMultiScan_vert.setLocalWorkSize(256,1);
			kernelMultiScan_vert.setRoundedGlobalWorkSize(256,imageWidth);
			kernelMultiScan_vert.setArg(0, _tempImage_cl);
			kernelMultiScan_vert.setArg(1, _imageIntegral_cl);
			_gpuComputeModule->queue().asyncRunKernel(kernelMultiScan_vert);
			_gpuComputeModule->queue().asyncCopyImageToBuffer(_imageIntegral_cl, _imageIntegralBuffer_cl);
		}
		else
		{
			cv::Mat srcHost(imageHeight, imageWidth, CV_8UC1), integralHost;
			_gpuComputeModule->queue().readImage2D(srcImage_cl, srcHost.data, srcHost.step);
			cv::integral(srcHost, integralHost);
			_gpuComputeModule->queue().writeImage2D(_imageIntegral_cl, integralHost.data, integralHost.step);
			_gpuComputeModule->queue().asyncCopyImageToBuffer(_imageIntegral_cl, _imageIntegralBuffer_cl);
		}
	}

	void buildScaleSpace(int imageWidth, int imageHeight) override
	{
		clw::Kernel kernelBuildScaleSpace = _gpuComputeModule->acquireKernel(_kidBuildScaleSpace);		

		for(int octave = 0; octave < _nOctaves; ++octave)
		{
			for(int scale = 0; scale < _nScales; ++scale)
			{
				auto&& layer = _scaleLayers[octave*_nScales + scale];

				if(imageWidth < layer.filterSize
					|| imageHeight < layer.filterSize)
					continue;

				// Promien filtru: dla 9x9 jest to 4
				int fw = (layer.filterSize - 1) / 2;
				// wielkosc 'garbu' (1/3 wielkosci filtru)
				int lobe = layer.filterSize / 3;
				// Promien poziomy (dla Lxx) garbu
				int lw = (lobe - 1) / 2;
				// Promien pionowy (dla Lxx) garbu
				int lh = lobe - 1;
				// Przesuniecie od 0,0 do centra garbu dla Lxy
				int xyOffset = lw + 1;

				// Odwrotnosc pola filtru - do normalizacji
				float invNorm = 1.0f / (layer.filterSize*layer.filterSize);

				// Wielkosc ramki dla ktorej nie bedzie liczone det(H) oraz tr(H)
				int layerMargin = layer.filterSize / (2 * layer.sampleStep);

				// Moze zdarzyc sie tak (od drugiej oktawy jest tak zawsze) ze przez sub-sampling
				// zaczniemy filtrowac od zbyt malego marginesu - ta wartosc (zwykle 1) pozwala tego uniknac
				int stepOffset = fw - (layerMargin * layer.sampleStep);

				// Ilosc probek w poziomie i pionie dla danej oktawy i skali
				int samples_x = 1 + (imageWidth - layer.filterSize) / layer.sampleStep;
				int samples_y = 1 + (imageHeight - layer.filterSize) / layer.sampleStep;

				/// TODO: Experiment with this value
				/// TODO: Experiment with PIX_PER_THREAD
				kernelBuildScaleSpace.setLocalWorkSize(16, 16);
				kernelBuildScaleSpace.setGlobalWorkOffset(layerMargin, layerMargin);
				kernelBuildScaleSpace.setRoundedGlobalWorkSize(samples_x, samples_y);
				kernelBuildScaleSpace.setArg( 0, _imageIntegralBuffer_cl);
				kernelBuildScaleSpace.setArg( 1, _integralBufferPitch);
				kernelBuildScaleSpace.setArg( 2, samples_x);
				kernelBuildScaleSpace.setArg( 3, samples_y);
				kernelBuildScaleSpace.setArg( 4, layer.hessian_cl);
				kernelBuildScaleSpace.setArg( 5, layer.laplacian_cl);
				kernelBuildScaleSpace.setArg( 6, layer.width);
				kernelBuildScaleSpace.setArg( 7, fw);
				kernelBuildScaleSpace.setArg( 8, lw);
				kernelBuildScaleSpace.setArg( 9, lh);
				kernelBuildScaleSpace.setArg(10, xyOffset);
				kernelBuildScaleSpace.setArg(11, invNorm);
				kernelBuildScaleSpace.setArg(12, layer.sampleStep);
				kernelBuildScaleSpace.setArg(13, stepOffset);

				_gpuComputeModule->queue().asyncRunKernel(kernelBuildScaleSpace);
			}
		}
	}

private:
	clw::Buffer  _imageIntegralBuffer_cl;
	int _integralBufferPitch;
};

REGISTER_NODE("OpenCL/Features/SURF Buffer", GpuSurfBufferNodeType)
REGISTER_NODE("OpenCL/Features/SURF", GpuSurfNodeType)

#endif
