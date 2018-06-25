/*
 * Copyright (c) 2013-2018 Kajetan Swierk <k0zmo@outlook.com>
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

#include "Logic/OpenCL/GpuNode.h"
#include "Logic/NodeFactory.h"

#include <fmt/core.h>

class GpuKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuKuwaharaFilterNodeType() : _radius(2)
    {
        addInput("Image", ENodeFlowDataType::DeviceImage);
        addOutput("Output", ENodeFlowDataType::DeviceImage);
        addProperty("Radius", _radius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 15))
            .setUiHints("min:1, max:15");
        setDescription("Kuwahara filter");
        setModule("opencl");
    }

    bool postInit() override
    {
        _kidKuwahara =
            _gpuComputeModule->registerKernel("kuwahara", "kuwahara.cl", "-DN_SECTORS=0");
        _kidKuwaharaRgb =
            _gpuComputeModule->registerKernel("kuwaharaRgb", "kuwahara.cl", "-DN_SECTORS=0");
        return _kidKuwahara != InvalidKernelID && _kidKuwaharaRgb != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
        // outputs
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

        // validate inputs
        if (deviceSrc.width() == 0 || deviceSrc.height() == 0)
            return ExecutionStatus(EStatus::Ok);

        int width = deviceSrc.width();
        int height = deviceSrc.height();
        clw::ImageFormat format = deviceSrc.format();

        // Prepare destination image and structuring element on a device
        ensureSizeIsEnough(deviceDest, width, height, format);

        KernelID kid = format.order == clw::EChannelOrder::R ? _kidKuwahara : _kidKuwaharaRgb;
        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, deviceDest);
        kernelKuwahara.setArg(2, (int)_radius);
        _gpuComputeModule->queue().runKernel(kernelKuwahara);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    void ensureSizeIsEnough(clw::Image2D& image, int width, int height,
                            const clw::ImageFormat& format)
    {
        if (image.isNull() || image.width() != width || image.height() != height ||
            image.format() != format)
        {
            image = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, format, width, height);
        }
    }

private:
    TypedNodeProperty<int> _radius;
    KernelID _kidKuwahara;
    KernelID _kidKuwaharaRgb;
};

REGISTER_NODE("OpenCL/Filters/Kuwahara filter", GpuKuwaharaFilterNodeType);
