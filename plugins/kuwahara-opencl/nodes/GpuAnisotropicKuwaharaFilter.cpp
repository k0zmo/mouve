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

#include "Logic/NodeSystem.h"
#include "Logic/OpenCL/GpuNode.h"

#include "impl/kuwahara.h"

#include <fmt/core.h>

class GpuAnisotropicKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuAnisotropicKuwaharaFilterNodeType()
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
        setDescription("Anisotropic Kuwahara filter");
        setModule("opencl");
    }

    bool postInit() override
    {
        string opts = fmt::format("-DN_SECTORS={}", _N);

        _kidAnisotropicKuwahara =
            _gpuComputeModule->registerKernel("anisotropicKuwahara", "kuwahara.cl", opts);
        _kidAnisotropicKuwaharaRgb =
            _gpuComputeModule->registerKernel("anisotropicKuwaharaRgb", "kuwahara.cl", opts);
        _kidCalcStructureTensor =
            _gpuComputeModule->registerKernel("calcStructureTensor", "kuwahara.cl", opts);
        _kidCalcStructureTensorRgb =
            _gpuComputeModule->registerKernel("calcStructureTensorRgb", "kuwahara.cl", opts);
        _kidConvolutionGaussian =
            _gpuComputeModule->registerKernel("convolutionGaussian", "kuwahara.cl", opts);
        _kidCalcOrientationAndAnisotropy =
            _gpuComputeModule->registerKernel("calcOrientationAndAnisotropy", "kuwahara.cl", opts);

        return _kidAnisotropicKuwahara != InvalidKernelID &&
               _kidAnisotropicKuwaharaRgb != InvalidKernelID &&
               _kidCalcStructureTensor != InvalidKernelID &&
               _kidCalcStructureTensorRgb != InvalidKernelID &&
               _kidConvolutionGaussian != InvalidKernelID &&
               _kidCalcOrientationAndAnisotropy != InvalidKernelID;
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
        ensureSizeIsEnough(_secondMomentMatrix, width, height,
                           clw::ImageFormat(clw::EChannelOrder::RGBA, clw::EChannelType::Float));
        ensureSizeIsEnough(_secondMomentMatrix2, width, height,
                           clw::ImageFormat(clw::EChannelOrder::RGBA, clw::EChannelType::Float));
        ensureSizeIsEnough(_oriAni, width, height,
                           clw::ImageFormat(clw::EChannelOrder::RG, clw::EChannelType::Float));

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

        clw::Kernel kernelCalcStructureTensor = _gpuComputeModule->acquireKernel(
            format.order == clw::EChannelOrder::R ? _kidCalcStructureTensor
                                                  : _kidCalcStructureTensorRgb);
        kernelCalcStructureTensor.setLocalWorkSize(clw::Grid(16, 16));
        kernelCalcStructureTensor.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelCalcStructureTensor.setArg(0, deviceSrc);
        kernelCalcStructureTensor.setArg(1, _secondMomentMatrix);
        _gpuComputeModule->queue().asyncRunKernel(kernelCalcStructureTensor);

        // const float sigmaSst = 2.0f;
        // cv::Mat gaussianKernel = cv::getGaussianKernel(11, sigmaSst, CV_32F);
        clw::Kernel kernelConvolutionGaussian =
            _gpuComputeModule->acquireKernel(_kidConvolutionGaussian);
        kernelConvolutionGaussian.setLocalWorkSize(clw::Grid(16, 16));
        kernelConvolutionGaussian.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelConvolutionGaussian.setArg(0, _secondMomentMatrix);
        kernelConvolutionGaussian.setArg(1, _secondMomentMatrix2);
        _gpuComputeModule->queue().asyncRunKernel(kernelConvolutionGaussian);

        clw::Kernel kernelCalcOrientationAndAnisotropy =
            _gpuComputeModule->acquireKernel(_kidCalcOrientationAndAnisotropy);
        kernelCalcOrientationAndAnisotropy.setLocalWorkSize(clw::Grid(16, 16));
        kernelCalcOrientationAndAnisotropy.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelCalcOrientationAndAnisotropy.setArg(0, _secondMomentMatrix2);
        kernelCalcOrientationAndAnisotropy.setArg(1, _oriAni);
        _gpuComputeModule->queue().asyncRunKernel(kernelCalcOrientationAndAnisotropy);

        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(
            format.order == clw::EChannelOrder::R ? _kidAnisotropicKuwahara
                                                  : _kidAnisotropicKuwaharaRgb);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, _kernelImage);
        kernelKuwahara.setArg(2, _oriAni);
        kernelKuwahara.setArg(3, deviceDest);
        kernelKuwahara.setArg(4, (int)_radius);
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

    KernelID _kidAnisotropicKuwahara;
    KernelID _kidAnisotropicKuwaharaRgb;
    KernelID _kidCalcStructureTensor;
    KernelID _kidCalcStructureTensorRgb;
    KernelID _kidConvolutionGaussian;
    KernelID _kidCalcOrientationAndAnisotropy;

    clw::Image2D _kernelImage;
    clw::Image2D _secondMomentMatrix;
    clw::Image2D _secondMomentMatrix2;
    clw::Image2D _oriAni;

    bool _recreateKernelImage;
};

void registerGpuAnisotropicKuwaharaFilter(NodeSystem& system)
{
    system.registerNodeType("OpenCL/Filters/Anisotropic Kuwahara filter",
                            makeDefaultNodeFactory<GpuAnisotropicKuwaharaFilterNodeType>());
}
