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

#include "impl/kuwahara.h"

#include <fmt/core.h>

class GpuGeneralizedKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuGeneralizedKuwaharaFilterNodeType()
        : _radius(6), _N(8), _smoothing(0.3333f), _recreateKernelImage(true)
    {
        addInput("Image", ENodeFlowDataType::DeviceImage);
        addOutput("Output", ENodeFlowDataType::DeviceImage);
        addProperty("Radius", _radius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 20))
            .setUiHints("min:1, max:20");
        addProperty("N", _N)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(3, 8))
            .setObserver(make_observer<FuncObserver>(
                [this](const NodeProperty&) { _recreateKernelImage = true; }))
            .setUiHints("min:3, max:8");
        addProperty("Smoothing", _smoothing)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setObserver(make_observer<FuncObserver>(
                [this](const NodeProperty&) { _recreateKernelImage = true; }))
            .setUiHints("min:0.00, max:1.00");
        setDescription("Generalized Kuwahara filter");
        setModule("opencl");
    }

    bool postInit() override
    {
        string opts = fmt::format("-DN_SECTORS={}", _N);

        _kidGeneralizedKuwahara =
            _gpuComputeModule->registerKernel("generalizedKuwahara", "kuwahara.cl", opts);
        _kidGeneralizedKuwaharaRgb =
            _gpuComputeModule->registerKernel("generalizedKuwaharaRgb", "kuwahara.cl", opts);
        return _kidGeneralizedKuwahara != InvalidKernelID &&
               _kidGeneralizedKuwaharaRgb != InvalidKernelID;
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

        if (_recreateKernelImage)
        {
            cv::Mat kernel_;
            cvu::getGeneralizedKuwaharaKernel(kernel_, _N, _smoothing);
            _kernelImage = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadOnly, clw::EMemoryLocation::Device,
                clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Float), kernel_.cols,
                kernel_.rows, kernel_.data);
            _recreateKernelImage = false;
        }

        KernelID kid = format.order == clw::EChannelOrder::R ? _kidGeneralizedKuwahara
                                                             : _kidGeneralizedKuwaharaRgb;
        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, deviceDest);
        kernelKuwahara.setArg(2, (int)_radius);
        kernelKuwahara.setArg(3, _kernelImage);
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
    TypedNodeProperty<int> _N;
    TypedNodeProperty<float> _smoothing;

    KernelID _kidGeneralizedKuwahara;
    KernelID _kidGeneralizedKuwaharaRgb;

    clw::Image2D _kernelImage;
    bool _recreateKernelImage;
};

REGISTER_NODE("OpenCL/Filters/Generalized Kuwahara filter", GpuGeneralizedKuwaharaFilterNodeType);
