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
#include "Kommon/Hash.h"

class GpuMorphologyOperatorNodeType : public GpuNodeType
{
public:
    GpuMorphologyOperatorNodeType()
        : _op(EMorphologyOperation::Erode)
        , _sElemHash(0)
    {
        addInput("Source", ENodeFlowDataType::DeviceImageMono);
        addInput("Structuring element", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::DeviceImageMono);
        addProperty("Operation type", _op)
            .setUiHints("item: Erode, item: Dilate, item: Open, item: Close,"
                "item: Gradient, item: Top Hat, item: Black Hat");
        setDescription("Performs a morphology operation on a given image");
        setModule("opencl");
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

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImageMono();
        const cv::Mat& sElem = reader.readSocket(1).getImageMono();
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImageMono();

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

        EMorphologyOperation op = _op.cast<Enum>().cast<EMorphologyOperation>();

        // Prepare if necessary buffer for temporary result
        if(op > EMorphologyOperation::Dilate)
            ensureSizeIsEnough(_tmpImage, width, height);

        // Acquire kernel(s)
        clw::Kernel kernelErode, kernelDilate, kernelSubtract;
        
        if(op != EMorphologyOperation::Dilate)
            kernelErode = _gpuComputeModule->acquireKernel(_kidErode);
        if(op > EMorphologyOperation::Erode)
            kernelDilate = _gpuComputeModule->acquireKernel(_kidDilate);
        //if(op > EMorphologyOperation::Close)
        //	kernelSubtract = _gpuComputeModule->acquireKernel(_kidSubtract);

        clw::Event evt;

        // Calculate metrics
        int kradx = (sElem.cols - 1) >> 1;
        int krady = (sElem.rows - 1) >> 1;		
        clw::Grid grid(deviceSrc.width(), deviceSrc.height());

        switch(op)
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

    TypedNodeProperty<EMorphologyOperation> _op;
    uint32_t _sElemHash;
};

REGISTER_NODE("OpenCL/Morphology/Operator", GpuMorphologyOperatorNodeType)

#endif
