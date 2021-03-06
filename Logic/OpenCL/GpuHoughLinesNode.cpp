/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

class GpuHoughLinesNodeType : public GpuNodeType
{
public:
    GpuHoughLinesNodeType()
        : _threshold(80)
        , _showHoughSpace(false)
        , _rhoResolution(1.0f)
        , _thetaResolution(1.0f)
        , _houghSpaceScale(100.0f)
    {
        addInput("Binary image", ENodeFlowDataType::DeviceImageMono);
        addOutput("Lines", ENodeFlowDataType::DeviceArray);
        addOutput("Hough space", ENodeFlowDataType::DeviceImageMono);
        addProperty("Threshold", _threshold)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Show Hough space", _showHoughSpace);
        addProperty("Rho resolution", _rhoResolution);
        addProperty("Theta resolution", _thetaResolution);
        addProperty("Hough space scale", _houghSpaceScale)
            .setValidator(make_validator<MinPropertyValidator<float>>(1.0f))
            .setUiHints("min:1.0");
        setModule("opencl");
    }

    bool postInit() override
    {
        _kidBuildPointsList = _gpuComputeModule->registerKernel("buildPointsList_basic", "hough.cl");
        _kidAccumLines = _gpuComputeModule->registerKernel("accumLines", "hough.cl");
        _kidAccumLinesShared = _gpuComputeModule->registerKernel("accumLines_shared", "hough.cl");
        _kidGetLines = _gpuComputeModule->registerKernel("getLines", "hough.cl");
        _kidAccumToImage = _gpuComputeModule->registerKernel("accumToImage", "hough.cl");
        _kidFillAccumSpace = _gpuComputeModule->registerKernel("fill_buffer_int", "fill.cl");

        return _kidBuildPointsList != InvalidKernelID
            && _kidAccumLines != InvalidKernelID
            && _kidAccumLinesShared != InvalidKernelID
            && _kidGetLines != InvalidKernelID
            && _kidAccumToImage != InvalidKernelID
            && _kidFillAccumSpace != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImageMono();
        DeviceArray& deviceLines = writer.acquireSocket(0).getDeviceArray();
        clw::Image2D& deviceAccumImage = writer.acquireSocket(1).getDeviceImageMono();

        int srcWidth = deviceImage.width();
        int srcHeight = deviceImage.height();

        if(srcWidth == 0 || srcHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        GpuPerformanceMarker marker(_gpuComputeModule->activityLogger(), "Hough");

        ensureSizeIsEnough(_deviceCounterPoints, sizeof(cl_uint));
        ensureSizeIsEnough(_deviceCounterLines, sizeof(cl_uint));

        // Zero global counters
        cl_uint zero = 0;
        _gpuComputeModule->queue().asyncWriteBuffer(_deviceCounterPoints, &zero);
        _gpuComputeModule->queue().asyncWriteBuffer(_deviceCounterLines, &zero);

        buildPointList(deviceImage, srcWidth, srcHeight);

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

        // Srednio 20% bialych pikseli (obrazu po detekcji krawedzi) i minimum 2 na linie
        cl_int maxLines = (cl_int) (srcWidth * srcHeight * 0.2 * 0.5);

        int linesCount = extractLines(deviceLines, maxLines, numRho, numAngle);

        if(_showHoughSpace)
            extractHoughSpace(deviceAccumImage, numAngle, numRho);
        else if(!deviceAccumImage.isNull())
            deviceAccumImage = clw::Image2D();

        return ExecutionStatus(EStatus::Ok, 
            string_format("Detected lines: %d (max: %d)", linesCount, maxLines));
    }

private:
    clw::Event buildPointList(const clw::Image2D& deviceImage, int width, int height) 
    {
        clw::Kernel kernelBuildPointsList = _gpuComputeModule->acquireKernel(_kidBuildPointsList);

        // Build list of non-zero pixels
        ensureSizeIsEnough(_devicePointsList, sizeof(cl_uint) * width * height);
        int pxPerThread = 1; /// TODO: parametrized it
        kernelBuildPointsList.setLocalWorkSize(clw::Grid(32, 4));
        kernelBuildPointsList.setRoundedGlobalWorkSize(clw::Grid((width + pxPerThread - 1)/pxPerThread, height));
        kernelBuildPointsList.setArg(0, deviceImage);
        kernelBuildPointsList.setArg(1, _devicePointsList);
        kernelBuildPointsList.setArg(2, _deviceCounterPoints);
        return _gpuComputeModule->queue().asyncRunKernel(kernelBuildPointsList);
    }

    void constructHoughSpace(int numRho, int numAngle) 
    {
        ensureSizeIsEnough(_deviceAccum, numAngle * numRho * sizeof(cl_int));

        size_t requiredSharedSize = numRho * sizeof(cl_int);
        float invRho = 1.0f / _rhoResolution;
        float theta = CL_M_PI_F/180.0f * _thetaResolution;

        if(!_gpuComputeModule->isLocalMemorySufficient(requiredSharedSize))
        {
            clw::Kernel kernelAccumLines = _gpuComputeModule->acquireKernel(_kidAccumLines);
            clw::Kernel kernelFillAccumSpace = _gpuComputeModule->acquireKernel(_kidFillAccumSpace);

            kernelFillAccumSpace.setLocalWorkSize(256);
            kernelFillAccumSpace.setRoundedGlobalWorkSize(numAngle * numRho);
            kernelFillAccumSpace.setArg(0, _deviceAccum);
            kernelFillAccumSpace.setArg(1, 0);
            kernelFillAccumSpace.setArg(2, numAngle * numRho);
            _gpuComputeModule->queue().asyncRunKernel(kernelFillAccumSpace);

            kernelAccumLines.setLocalWorkSize(clw::Grid(256));
            kernelAccumLines.setRoundedGlobalWorkSize(clw::Grid(256 * numAngle));
            kernelAccumLines.setArg(0, _devicePointsList);
            kernelAccumLines.setArg(1, _deviceCounterPoints);
            kernelAccumLines.setArg(2, _deviceAccum);
            kernelAccumLines.setArg(3, numRho);
            kernelAccumLines.setArg(4, invRho);
            kernelAccumLines.setArg(5, theta);
            _gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
        }
        else
        {
            clw::Kernel kernelAccumLines = _gpuComputeModule->acquireKernel(_kidAccumLinesShared);

            kernelAccumLines.setLocalWorkSize(clw::Grid(256));
            kernelAccumLines.setRoundedGlobalWorkSize(clw::Grid(256 * numAngle));
            kernelAccumLines.setArg(0, _devicePointsList);
            kernelAccumLines.setArg(1, _deviceCounterPoints);
            kernelAccumLines.setArg(2, _deviceAccum);
            kernelAccumLines.setArg(3, clw::LocalMemorySize(requiredSharedSize));
            kernelAccumLines.setArg(4, numRho);
            kernelAccumLines.setArg(5, invRho);
            kernelAccumLines.setArg(6, theta);
            _gpuComputeModule->queue().asyncRunKernel(kernelAccumLines);
        }
    }

    int extractLines(DeviceArray &deviceLines, int maxLines, int numRho, int numAngle) 
    {
        clw::Kernel kernelGetLines = _gpuComputeModule->acquireKernel(_kidGetLines);

        if(deviceLines.isNull()
        || deviceLines.size() !=  maxLines * sizeof(cl_float2))
        {
            deviceLines = DeviceArray::create(_gpuComputeModule->context(), clw::EAccess::ReadWrite,
                clw::EMemoryLocation::Device, 2, maxLines, EDataType::Float);
        }

        float theta = CL_M_PI_F/180.0f * _thetaResolution;

        kernelGetLines.setLocalWorkSize(clw::Grid(32, 8));
        kernelGetLines.setRoundedGlobalWorkSize(clw::Grid(numRho, numAngle));
        kernelGetLines.setArg(0, _deviceAccum);
        kernelGetLines.setArg(1, deviceLines.buffer());
        kernelGetLines.setArg(2, _deviceCounterLines);
        kernelGetLines.setArg(3, _threshold.cast_value<int>());
        kernelGetLines.setArg(4, maxLines);
        kernelGetLines.setArg(5, numRho);
        kernelGetLines.setArg(6, numAngle);
        kernelGetLines.setArg(7, (float) _rhoResolution);
        kernelGetLines.setArg(8, theta);
        _gpuComputeModule->queue().asyncRunKernel(kernelGetLines);

        // Read results
        cl_uint linesCount;
        cl_uint* linesCountPtr = (cl_uint*) _gpuComputeModule->queue().mapBuffer(_deviceCounterLines, clw::EMapAccess::Read);
        linesCount = *linesCountPtr;
        _gpuComputeModule->queue().asyncUnmap(_deviceCounterLines, linesCountPtr);

        deviceLines.truncate(linesCount);

        return linesCount;
    }

    void extractHoughSpace(clw::Image2D& deviceAccumImage, int numAngle, int numRho) 
    {
        ensureSizeIsEnough(deviceAccumImage, numAngle, numRho);

        clw::Kernel kernelAccumToImage = _gpuComputeModule->acquireKernel(_kidAccumToImage);
        kernelAccumToImage.setLocalWorkSize(16, 16);
        kernelAccumToImage.setRoundedGlobalWorkSize(numRho, numAngle);
        kernelAccumToImage.setArg(0, _deviceAccum);
        kernelAccumToImage.setArg(1, numRho);
        kernelAccumToImage.setArg(2, _houghSpaceScale.cast_value<float>());
        kernelAccumToImage.setArg(3, deviceAccumImage);
        _gpuComputeModule->queue().runKernel(kernelAccumToImage);
    }

    void ensureSizeIsEnough(clw::Buffer& buffer, size_t size)
    {
        /// TODO: Or buffer.size() < size
        if(buffer.isNull() || buffer.size() != size)
        {
            buffer = _gpuComputeModule->context().createBuffer(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, size);
        }
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

private:
    TypedNodeProperty<int> _threshold;
    TypedNodeProperty<bool> _showHoughSpace;
    TypedNodeProperty<float> _rhoResolution;
    TypedNodeProperty<float> _thetaResolution;
    TypedNodeProperty<float> _houghSpaceScale;

    clw::Buffer _deviceCounterPoints;
    clw::Buffer _deviceCounterLines;
    clw::Buffer _devicePointsList;
    clw::Buffer _deviceAccum;

    KernelID _kidBuildPointsList;
    KernelID _kidAccumLines;
    KernelID _kidAccumLinesShared;
    KernelID _kidGetLines;
    KernelID _kidAccumToImage;
    KernelID _kidFillAccumSpace;
};

REGISTER_NODE("OpenCL/Features/Hough Lines", GpuHoughLinesNodeType)

#endif
